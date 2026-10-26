import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    DEVICE_CLASS_CARBON_DIOXIDE,
    STATE_CLASS_MEASUREMENT,
    UNIT_PARTS_PER_MILLION,
)

from . import CubicCo2Sensor

CONFIG_SCHEMA = sensor.sensor_schema(
    CubicCo2Sensor,
    unit_of_measurement=UNIT_PARTS_PER_MILLION,
    accuracy_decimals=0,
    device_class=DEVICE_CLASS_CARBON_DIOXIDE,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(uart.UART_DEVICE_SCHEMA)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "cubic_co2", baud_rate=9600, require_rx=True
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
