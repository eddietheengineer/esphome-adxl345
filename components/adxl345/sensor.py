import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    UNIT_DEGREES,
    UNIT_G,
)
from . import ADXL345, adxl345_ns, DEPENDENCIES

DEPENDENCIES += ["adxl345"]

CONF_AXIS = "axis"
CONF_PARENT_ID = "adxl345_id"

# ---------------------------------------------------------------------------
# Acceleration and tilt sensors.
#
# The `axis` field selects which measurement to expose:
#   x, y, z, magnitude  -> acceleration in g
#   tilt_x, tilt_y, tilt_z -> tilt angle in degrees
#
# The `adxl345_id` field references the ADXL345 component.
# ---------------------------------------------------------------------------

# Tilt axes report an angle in degrees; the acceleration axes report g.
TILT_AXES = {"tilt_x", "tilt_y", "tilt_z"}

# Vibration-analysis axes and their units.
VIBRATION_UNITS = {
    "vibration_frequency": "Hz",
    "vibration_amplitude": "g",
    "vibration_deflection": "mm",
    "vibration_peak": "g",
}


def _default_unit(config):
    """Default the unit per axis: degrees for tilt, g for acceleration, and
    Hz / g / mm for the vibration-analysis outputs."""
    if "unit_of_measurement" not in config:
        axis = config[CONF_AXIS]
        if axis in VIBRATION_UNITS:
            config["unit_of_measurement"] = VIBRATION_UNITS[axis]
        elif axis in TILT_AXES:
            config["unit_of_measurement"] = UNIT_DEGREES
        else:
            config["unit_of_measurement"] = UNIT_G
    return config
CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(sensor.Sensor),
            cv.Required(CONF_NAME): cv.string,
            cv.Required(CONF_AXIS): cv.enum(
                {
                    "x": "x",
                    "y": "y",
                    "z": "z",
                    "magnitude": "magnitude",
                    "tilt_x": "tilt_x",
                    "tilt_y": "tilt_y",
                    "tilt_z": "tilt_z",
                    "vibration_frequency": "vibration_frequency",
                    "vibration_amplitude": "vibration_amplitude",
                    "vibration_deflection": "vibration_deflection",
                    "vibration_peak": "vibration_peak",
                },
                lower=True,
            ),
            cv.Required(CONF_PARENT_ID): cv.use_id(ADXL345),
            cv.Optional("unit_of_measurement"): cv.string,
            cv.Optional("accuracy_decimals", default=3): cv.int_,
            cv.Optional("icon"): cv.icon,
            cv.Optional("device_class"): cv.string,
            cv.Optional("state_class"): cv.string,
            cv.Optional("entity_category"): cv.string,
            cv.Optional("filters"): cv.string,
            cv.Optional("internal", default=False): cv.boolean,
            cv.Optional("disabled_by_default", default=False): cv.boolean,
            cv.Optional("force_update", default=False): cv.boolean,
        }
    ),
    _default_unit,
)


async def to_code(config):
    # Create the sensor Pvariable.
    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)

    # Get the ADXL345 parent component.
    parent = await cg.get_variable(config[CONF_PARENT_ID])

    axis = config[CONF_AXIS]
    if axis == "x":
        cg.add(cg.RawExpression(f"{parent}->set_x_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "y":
        cg.add(cg.RawExpression(f"{parent}->set_y_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "z":
        cg.add(cg.RawExpression(f"{parent}->set_z_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "magnitude":
        cg.add(cg.RawExpression(f"{parent}->set_magnitude_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "tilt_x":
        cg.add(cg.RawExpression(f"{parent}->set_tilt_x_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "tilt_y":
        cg.add(cg.RawExpression(f"{parent}->set_tilt_y_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "tilt_z":
        cg.add(cg.RawExpression(f"{parent}->set_tilt_z_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "vibration_frequency":
        cg.add(cg.RawExpression(f"{parent}->set_vibration_frequency_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "vibration_amplitude":
        cg.add(cg.RawExpression(f"{parent}->set_vibration_amplitude_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "vibration_deflection":
        cg.add(cg.RawExpression(f"{parent}->set_vibration_deflection_callback([&](float val) {{ {var}->publish_state(val); }});"))
    elif axis == "vibration_peak":
        cg.add(cg.RawExpression(f"{parent}->set_vibration_peak_callback([&](float val) {{ {var}->publish_state(val); }});"))
