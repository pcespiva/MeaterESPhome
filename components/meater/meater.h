#pragma once

#include "esphome.h"
d
class MeaterSensor : public PollingComponent, public sensor::Sensor, public BLEClientNode {
 public:
  MeaterSensor() : PollingComponent(15000) {}
  void setup() override;
  void update() override;
  void set_mac(const std::string &mac);
  void set_identifier(const std::string &id);
  void set_connect_on_demand(bool v);
  void set_ble_client(ble_client::BLEClient *c);
  void set_notify_timeout_ms(uint32_t ms);
  void set_tip_sensor(sensor::Sensor *s);
  void set_ambient_sensor(sensor::Sensor *s);
  void set_battery_sensor(sensor::Sensor *s);
  void set_firmware_text_sensor(text_sensor::TextSensor *t);
};
