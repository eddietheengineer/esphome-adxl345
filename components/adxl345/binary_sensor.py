import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
)
from . import ADXL345, adxl345_ns, DEPENDENCIES

DEPENDENCIES += ["adxl345"]

CONF_SOURCE = "source"
CONF_PARENT_ID = "adxl345_id"

# ---------------------------------------------------------------------------
# Interrupt / event binary sensors.
#
# The ADXL345 reports events through its INT_SOURCE register (0x30). The
# driver polls this register and fires edge-triggered callbacks for each
# event type. These binary sensors surface those events to Home Assistant.
# ---------------------------------------------------------------------------
CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(binary_sensor.BinarySensor),
            cv.Required(CONF_NAME): cv.string,
            cv.Required(CONF_SOURCE): cv.enum(
                {
                    "data_ready": "data_ready",
                    "single_tap": "single_tap",
                    "double_tap": "double_tap",
                    "activity": "activity",
                    "inactivity": "inactivity",
                    "free_fall": "free_fall",
                },
                lower=True,
            ),
            cv.Required(CONF_PARENT_ID): cv.use_id(ADXL345),
            cv.Optional("icon"): cv.icon,
            cv.Optional("device_class"): cv.string,
            cv.Optional("entity_category"): cv.string,
            cv.Optional("internal", default=False): cv.boolean,
            cv.Optional("disabled_by_default", default=False): cv.boolean,
            cv.Optional("force_update", default=False): cv.boolean,
        }
    ),
)

# Map each source to the C++ setter method name on the ADXL345 class.
_SOURCE_TO_SETTER = {
    "data_ready": "set_data_ready_callback",
    "single_tap": "set_single_tap_callback",
    "double_tap": "set_double_tap_callback",
    "activity": "set_activity_callback",
    "inactivity": "set_inactivity_callback",
    "free_fall": "set_free_fall_callback",
}

# Sources whose C++ callback receives the current level (bool) of the
# interrupt bit and publishes it every cycle. data_ready is the exception:
# its callback takes no argument and publishes true when the bit is set.
_LEVEL_SOURCES = {"single_tap", "double_tap", "activity", "inactivity", "free_fall"}


async def to_code(config):
    # Create the binary sensor Pvariable.
    var = cg.new_Pvariable(config[CONF_ID])
    await binary_sensor.register_binary_sensor(var, config)

    # Get the ADXL345 parent component.
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    source = config[CONF_SOURCE]
    setter = _SOURCE_TO_SETTER[source]

    # Build the C++ statement. Level sources receive the current bit state
    # (bool) and publish it; data_ready publishes true when its bit is set.
    if source in _LEVEL_SOURCES:
        stmt = f"{parent}->{setter}([&](bool val) {{ {var}->publish_state(val); }});"
    else:
        stmt = f"{parent}->{setter}([&] {{ {var}->publish_state(true); }});"
    cg.add(cg.RawExpression(stmt))
