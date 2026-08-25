import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_NAME,
)

DEPENDENCIES = ["i2c"]

adxl345_ns = cg.esphome_ns.namespace("adxl345")
ADXL345 = adxl345_ns.class_("ADXL345", cg.PollingComponent, i2c.I2CDevice)

CONF_VIBRATION = "vibration"

# ---------------------------------------------------------------------------
# Data-rate presets (BW_RATE register, 0x2C).
# ---------------------------------------------------------------------------
DATA_RATE_PRESETS = {
    "6.25 Hz": 0x06,
    "12.5 Hz": 0x07,
    "25 Hz": 0x08,
    "50 Hz": 0x09,
    "100 Hz": 0x0A,
    "200 Hz": 0x0B,
    "400 Hz": 0x0C,
    "800 Hz": 0x0D,
    "1600 Hz": 0x0E,
    "3200 Hz": 0x0F,
}

# ---------------------------------------------------------------------------
# g-range presets (DATA_FORMAT range bits, 0x31).
# ---------------------------------------------------------------------------
RANGE_PRESETS = {
    "2 g": 0x00,
    "4 g": 0x01,
    "8 g": 0x02,
    "16 g": 0x03,
}

# ---------------------------------------------------------------------------
# FIFO mode presets (FIFO_CTL bits 7:6).
# ---------------------------------------------------------------------------
FIFO_MODE_PRESETS = {
    "bypass": 0x00,
    "fifo": 0x01,
    "stream": 0x02,
    "trigger": 0x03,
}

# ---------------------------------------------------------------------------
# Sleep-mode wakeup-rate presets (POWER_CTL bits 1:0).
# ---------------------------------------------------------------------------
WAKEUP_PRESETS = {
    "8 Hz": 0x00,
    "4 Hz": 0x01,
    "2 Hz": 0x02,
    "1 Hz": 0x03,
}


def _rate_hz(rate):
    """'1000 Hz' -> 1000.0 (the numeric part of a data-rate preset name)."""
    return float(str(rate).split()[0])


def _odr_code_for(rate_hz):
    """Smallest BW_RATE preset at or above rate_hz, so each poll is a fresh sample."""
    best_code = None
    best_hz = None
    for name, code in DATA_RATE_PRESETS.items():
        hz = _rate_hz(name)
        if hz >= rate_hz and (best_hz is None or hz < best_hz):
            best_hz = hz
            best_code = code
    if best_code is None:
        best_code = max(DATA_RATE_PRESETS.values())
    return best_code


def _snap_pow2(n):
    """Nearest power of two to n (minimum 2), for the FFT window size."""
    if n <= 2:
        return 2
    lower = 1
    while lower * 2 <= n:
        lower *= 2
    upper = lower * 2
    return upper if (n - lower) < (upper - n) else lower


def _validate_data_rate(value):
    if value in DATA_RATE_PRESETS:
        return value
    raise cv.Invalid(
        f"Invalid data rate '{value}'. Valid values: {', '.join(DATA_RATE_PRESETS)}"
    )


def _validate_range(value):
    if value in RANGE_PRESETS:
        return value
    raise cv.Invalid(
        f"Invalid range '{value}'. Valid values: {', '.join(RANGE_PRESETS)}"
    )


def _validate_fifo_mode(value):
    if value in FIFO_MODE_PRESETS:
        return value
    raise cv.Invalid(
        f"Invalid FIFO mode '{value}'. Valid values: {', '.join(FIFO_MODE_PRESETS)}"
    )


def _validate_wakeup(value):
    if value in WAKEUP_PRESETS:
        return value
    raise cv.Invalid(
        f"Invalid wakeup rate '{value}'. Valid values: {', '.join(WAKEUP_PRESETS)}"
    )


# ---------------------------------------------------------------------------
# Main component schema.
#
# I2C bus parameters (i2c_id, address) come from i2c_device_schema(). The
# ADXL345 has a fixed 7-bit address of 0x53. ADXL345-specific options are
# defined here.
# ---------------------------------------------------------------------------
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ADXL345),
            cv.Required(CONF_NAME): cv.string,
            cv.Optional("output_rate", default="100 Hz"): cv.All(
                cv.string, _validate_data_rate
            ),
            cv.Optional("range", default="4 g"): cv.All(cv.string, _validate_range),
            cv.Optional("full_resolution", default=True): cv.boolean,
            cv.Optional("low_power", default=False): cv.boolean,
            cv.Optional("self_test", default=False): cv.boolean,
            cv.Optional("int_invert", default=False): cv.boolean,
            cv.Optional("justify", default=False): cv.boolean,
            cv.Optional("wakeup", default="8 Hz"): cv.All(
                cv.string, _validate_wakeup
            ),
            cv.Optional("auto_sleep", default=False): cv.boolean,
            cv.Optional("link", default=False): cv.boolean,
            cv.Optional("sleep", default=False): cv.boolean,
            cv.Optional("fifo_mode", default="bypass"): cv.All(
                cv.string, _validate_fifo_mode
            ),
            cv.Optional("fifo_samples", default=0): cv.int_range(0, 31),
            cv.Optional("fifo_trigger", default=False): cv.boolean,
            cv.Optional("threshold_tap", default=0): cv.int_range(0, 255),
            cv.Optional("threshold_activity", default=0): cv.int_range(0, 255),
            cv.Optional("threshold_inactivity", default=0): cv.int_range(0, 255),
            cv.Optional("threshold_free_fall", default=0): cv.int_range(0, 255),
            cv.Optional("time_free_fall", default=0): cv.int_range(0, 255),
            cv.Optional("tap_duration", default=0): cv.int_range(0, 255),
            cv.Optional("tap_latency", default=0): cv.int_range(0, 255),
            cv.Optional("tap_window", default=0): cv.int_range(0, 255),
            cv.Optional("time_inactivity", default=0): cv.int_range(0, 255),
            cv.Optional("tap_axes", default=0): cv.int_range(0, 255),
            cv.Optional("act_inact_ctl", default=0): cv.int_range(0, 255),
            cv.Optional("int_enable", default=0): cv.int_range(0, 255),
            cv.Optional("int_map", default=0): cv.int_range(0, 255),
            cv.Optional("offset_x", default=0): cv.int_range(-128, 127),
            cv.Optional("offset_y", default=0): cv.int_range(-128, 127),
            cv.Optional("offset_z", default=0): cv.int_range(-128, 127),
            cv.Optional(CONF_VIBRATION): cv.Schema(
                {
                    cv.Optional("axis", default="y"): cv.enum(
                        {"x": "x", "y": "y", "z": "z"}, lower=True
                    ),
                    cv.Optional("sample_rate", default=1000): cv.int_range(100, 1000),
                    cv.Optional("window", default="2s"): cv.positive_time_period,
                    cv.Optional("min_frequency", default=1.0): cv.positive_float,
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("100ms"))
    .extend(i2c.i2c_device_schema(0x53))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    # Sensor output data rate (BW_RATE register).
    cg.add(var.set_data_rate_code(DATA_RATE_PRESETS[config["output_rate"]]))
    # Range.
    cg.add(var.set_range(RANGE_PRESETS[config["range"]]))
    # Data format options.
    cg.add(var.set_full_resolution(config["full_resolution"]))
    cg.add(var.set_low_power(config["low_power"]))
    cg.add(var.set_self_test(config["self_test"]))
    cg.add(var.set_int_invert(config["int_invert"]))
    cg.add(var.set_justify(config["justify"]))
    cg.add(var.set_wakeup_bits(WAKEUP_PRESETS[config["wakeup"]]))
    # Power control options.
    cg.add(var.set_auto_sleep(config["auto_sleep"]))
    cg.add(var.set_link(config["link"]))
    cg.add(var.set_sleep(config["sleep"]))
    # FIFO options.
    cg.add(var.set_fifo_mode(FIFO_MODE_PRESETS[config["fifo_mode"]]))
    cg.add(var.set_fifo_samples(config["fifo_samples"]))
    cg.add(var.set_fifo_trigger(config["fifo_trigger"]))
    # Interrupt threshold / timing.
    cg.add(var.set_thresh_tap(config["threshold_tap"]))
    cg.add(var.set_thresh_act(config["threshold_activity"]))
    cg.add(var.set_thresh_inact(config["threshold_inactivity"]))
    cg.add(var.set_thresh_ff(config["threshold_free_fall"]))
    cg.add(var.set_time_ff(config["time_free_fall"]))
    cg.add(var.set_dur(config["tap_duration"]))
    cg.add(var.set_latent(config["tap_latency"]))
    cg.add(var.set_window(config["tap_window"]))
    cg.add(var.set_time_inact(config["time_inactivity"]))
    cg.add(var.set_tap_axes(config["tap_axes"]))
    cg.add(var.set_act_inact_ctl(config["act_inact_ctl"]))
    cg.add(var.set_int_enable(config["int_enable"]))
    cg.add(var.set_int_map(config["int_map"]))
    # Per-axis offsets.
    cg.add(var.set_offset_x(config["offset_x"]))
    cg.add(var.set_offset_y(config["offset_y"]))
    cg.add(var.set_offset_z(config["offset_z"]))

    # Vibration analysis: override the ODR and poll rate for ~1 kHz sampling
    # and hand the FFT window parameters to the driver.
    if CONF_VIBRATION in config:
        vib = config[CONF_VIBRATION]
        sample_rate = float(vib["sample_rate"])
        window_s = vib["window"].total_nanoseconds / 1e9
        window_samples = _snap_pow2(int(window_s * sample_rate))
        axis_idx = {"x": 0, "y": 1, "z": 2}[vib["axis"]]
        cg.add(var.set_data_rate_code(_odr_code_for(sample_rate)))
        cg.add(var.enable_vibration(axis_idx, window_samples, float(vib["min_frequency"]), sample_rate))
        cg.add(var.set_update_interval(max(1, int(1000 / sample_rate))))
