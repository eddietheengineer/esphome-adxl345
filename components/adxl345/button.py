import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from . import ADXL345, adxl345_ns

DEPENDENCIES = ["adxl345"]

CONF_ADXL345_ID = "adxl345_id"

# The C++ class is defined in adxl345.h (the component's main header), so it
# is always pulled in alongside the ADXL345 driver.
DumpButton = adxl345_ns.class_("DumpButton", button.Button)

CONFIG_SCHEMA = button.button_schema(
    DumpButton,
    icon="mdi:file-csv-outline",
).extend(
    {
        cv.Required(CONF_ADXL345_ID): cv.use_id(ADXL345),
    }
)


async def to_code(config):
    var = await button.new_button(config)
    parent = await cg.get_variable(config[CONF_ADXL345_ID])
    cg.add(var.set_parent(parent))
    return var
