#include "meater.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include <string>

MeaterSensor *meater_instance = nullptr;
d
void MeaterSensor::setup() {
  // Initialization: do not connect automatically if connect_on_demand==true
  ESP_LOGD("meater", "setup called; connect_on_demand=%s", connect_on_demand ? "true" : "false");
}

void MeaterSensor::update() {
  // Periodic update - publish a test value for now
  this->publish_state(0.0);
}

void MeaterSensor::request_connect() {
  if (connect_on_demand && connected) {
    ESP_LOGD("meater", "request_connect: already connected");
    return;
  }

  ESP_LOGI("meater", "request_connect(): attempting connect (mac=%s, id=%s)",
           mac_address.c_str(), identifier.c_str());

  if (ble_client_ptr) {
    ESP_LOGI("meater", "Using provided ble_client to connect");
    ble_client_ptr->connect();
    connected = true;
    this->got_tip_ = false;
    this->got_ambient_ = false;
    if (this->notify_timeout_ms > 0) {
      this->set_timeout("meater_notify_timeout", this->notify_timeout_ms,
                        [this]() { ESP_LOGW("meater", "Notify timeout reached, disconnecting"); this->request_disconnect(); });
    }
  } else {
    ESP_LOGW("meater", "No ble_client assigned; cannot connect");
  }
}

void MeaterSensor::request_disconnect() {
  if (!connected) {
    ESP_LOGD("meater", "request_disconnect: not connected");
    return;
  }
  if (ble_client_ptr) {
    ESP_LOGI("meater", "Disconnecting ble_client");
    ble_client_ptr->disconnect();
    this->set_timeout("meater_disconnect", 0, []() {});
    this->set_timeout("meater_notify_timeout", 0, []() {});
    this->got_tip_ = false;
    this->got_ambient_ = false;
  }
  connected = false;
}

void MeaterSensor::set_ble_client(esphome::ble_client::BLEClient *client) {
  this->ble_client_ptr = client;
  if (client) {
    client->register_ble_node(this);
  }
}

bool discover_meater_characteristics(MeaterSensor *self) {
  namespace espbt = esphome::esp32_ble_tracker;
  static const espbt::ESPBTUUID MEATER_SERVICE = espbt::ESPBTUUID::from_raw("a75cc7fc-c956-488f-ac2a-2dbc08b63a04");
  static const espbt::ESPBTUUID MEATER_CHAR_TEMP = espbt::ESPBTUUID::from_raw("7edda774-045e-4bbf-909b-45d1991a2876");
  static const espbt::ESPBTUUID MEATER_CHAR_BAT = espbt::ESPBTUUID::from_raw("2adb4877-68d8-4884-bd3c-d83853bf27b8");
  static const espbt::ESPBTUUID FIRMWARE_CHAR = espbt::ESPBTUUID::from_raw("00002a26-0000-1000-8000-00805f9b34fb");

  if (!self->char_handle_tip_) {
    auto *chr = self->parent()->get_characteristic(MEATER_SERVICE, MEATER_CHAR_TEMP);
    if (chr == nullptr) {
      ESP_LOGW("meater", "No temp characteristic found");
      return false;
    }
    self->char_handle_tip_ = chr->handle;
  }

  if (!self->char_handle_battery_) {
    auto *chr = self->parent()->get_characteristic(MEATER_SERVICE, MEATER_CHAR_BAT);
    if (chr == nullptr) {
      ESP_LOGW("meater", "No battery characteristic found");
      return false;
    }
    self->char_handle_battery_ = chr->handle;
  }

  if (!self->char_handle_firmware_) {
    auto *chr = self->parent()->get_characteristic(espbt::ESPBTUUID::from_uint16(0x180A), FIRMWARE_CHAR);
    if (chr != nullptr) {
      self->char_handle_firmware_ = chr->handle;
    }
  }

  if (!self->config_descr_tip_) {
    auto *descr = self->parent()->get_config_descriptor(self->char_handle_tip_);
    if (descr) {
      self->config_descr_tip_ = descr->handle;
    } else {
      ESP_LOGW("meater", "No config descriptor for temp char; notifications may not work");
    }
  }

  ESP_LOGI("meater", "Meater characteristics discovered: temp=0x%x bat=0x%x fw=0x%x descr=0x%x",
           self->char_handle_tip_, self->char_handle_battery_, self->char_handle_firmware_, self->config_descr_tip_);
  return true;
}

void MeaterSensor::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                     esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGD("meater", "Service discovery complete, discovering characteristics");
      if (discover_meater_characteristics(this)) {
        this->node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
        if (this->config_descr_tip_) {
          auto status = esp_ble_gattc_register_for_notify(this->parent()->get_gattc_if(), this->parent()->get_remote_bda(), this->char_handle_tip_);
          ESP_LOGD("meater", "register_for_notify status=%d", status);
        }
      } else {
        ESP_LOGW("meater", "Failed discovering Meater characteristics");
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      uint16_t handle = param->notify.handle;
      uint8_t *val = param->notify.value;
      uint16_t len = param->notify.value_len;

      if (handle == this->char_handle_tip_) {
        if (len >= 2) {
          float tip_temp = (val[0] + (val[1] << 8) + 8.0f) / 16.0f;
          this->tip_cache_ = tip_temp;
          this->got_tip_ = true;
          if (this->tip_sensor_) this->tip_sensor_->publish_state(tip_temp);
        }
        if (len >= 6) {
          uint16_t tip = val[0] + (val[1] << 8);
          uint16_t ra = val[2] + (val[3] << 8);
          uint16_t oa = val[4] + (val[5] << 8);
          uint16_t min_val = 48;
          float ambient = (tip + std::max(0, (((int)ra - std::min((int)min_val, (int)oa)) * 16 * 589) / 1487) + 8.0f) / 16.0f;
          this->ambient_cache_ = ambient;
          this->got_ambient_ = true;
          if (this->ambient_sensor_) this->ambient_sensor_->publish_state(ambient);
        }
      } else if (handle == this->char_handle_battery_) {
        if (len >= 2) {
          uint16_t battery = (val[0] + val[1]) * 10;
          this->battery_cache_ = (float)battery;
          if (this->battery_sensor_) this->battery_sensor_->publish_state((float)battery);
        }
      } else if (handle == this->char_handle_firmware_) {
        std::string data_string((char *)val, (size_t)len);
        if (this->firmware_text_) this->firmware_text_->publish_state(data_string.c_str());
      }

      if (this->got_tip_ && this->got_ambient_) {
        this->set_timeout("meater_notify_timeout", 0, []() {});
        this->set_timeout("meater_disconnect", 1000, [this]() { this->request_disconnect(); });
      }
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGI("meater", "Disconnected event received");
      this->connected = false;
      break;
    }
    default:
      break;
  }
}
