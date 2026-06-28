import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, sensor, text_sensor
from esphome.const import (
    CONF_ID,
)
d
DEPENDENCIES = ["ble_client"]
AUTO_LOAD = ["ble_client"]

meater_ns = cg.esphome_ns.namespace("meater")
MeaterSensor = meater_ns.class_("MeaterSensor", cg.PollingComponent)

CONF_TIP = "tip"
CONF_AMBIENT = "ambient"
CONF_BATTERY = "battery"
CONF_FIRMWARE = "firmware"
CONF_NOTIFY_TIMEOUT = "notify_timeout"

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(MeaterSensor),
        cv.Optional(CONF_NOTIFY_TIMEOUT): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TIP): sensor.sensor_schema(unit_of_measurement="°C", accuracy_decimals=2),
        cv.Optional(CONF_AMBIENT): sensor.sensor_schema(unit_of_measurement="°C", accuracy_decimals=2),
        cv.Optional(CONF_BATTERY): sensor.sensor_schema(unit_of_measurement="%", accuracy_decimals=0),
        cv.Optional(CONF_FIRMWARE): text_sensor.text_sensor_schema(),
    }
).extend(ble_client.BLE_CLIENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

    if CONF_NOTIFY_TIMEOUT in config:
        cg.add(var.set_notify_timeout_ms(config[CONF_NOTIFY_TIMEOUT]))

    if CONF_TIP in config:
        sens = await sensor.new_sensor(config[CONF_TIP])
        cg.add(var.set_tip_sensor(sens))
    if CONF_AMBIENT in config:
        sens = await sensor.new_sensor(config[CONF_AMBIENT])
        cg.add(var.set_ambient_sensor(sens))
    if CONF_BATTERY in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY])
        cg.add(var.set_battery_sensor(sens))
    if CONF_FIRMWARE in config:
        txt = await text_sensor.new_text_sensor(config[CONF_FIRMWARE])
        cg.add(var.set_firmware_text_sensor(txt))
# Python init for compatibility (ESPHome components are C++).
