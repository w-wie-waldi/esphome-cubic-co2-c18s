import esphome.codegen as cg
from esphome.components import uart

CODEOWNERS = ["@w-wie-waldi"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]

cubic_co2_ns = cg.esphome_ns.namespace("cubic_co2")

CubicCo2Sensor = cubic_co2_ns.class_(
    "CubicCo2Sensor", cg.Component, uart.UARTDevice
)
