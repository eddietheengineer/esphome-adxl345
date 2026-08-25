# ADXL345 ESPHome External Component

A 3-axis accelerometer driver for the [Analog Devices ADXL345](https://www.analog.com/en/products/adxl345.html), connected via **I2C**. Exposes X/Y/Z acceleration, vector magnitude, tilt angles, vibration analysis (dominant frequency, amplitude, deflection, peak), and interrupt events (tap, double-tap, activity, inactivity, free-fall) as Home Assistant sensors.

## Hardware

| Item | Notes |
|------|-------|
| **Sensor** | A standard **ADXL345 breakout board** (e.g. the common 2×8 header modules sold by Adafruit, SparkFun, DFRobot, and many AliExpress/Amazon sellers). These boards already include the ADXL345, the power regulator, and the decoupling capacitors, so no extra parts are needed. |
| **MCU** | Any ESP32 / ESP8266 / RP2040 with I2C. The example below uses the **Seeed XIAO ESP32-C3**. |
| **Supply** | 3.3 V (the ADXL345 operates at 2.0–3.6 V; 3.3 V is standard for ESP boards) |

> **Note:** The ADXL345 has a **fixed I2C address of 0x53** — there is no address pin. The breakout's `SDO` and `CS` pins are SPI-only and are left unconnected in I2C mode.

## Wiring — XIAO ESP32-C3 → ADXL345 breakout

Most ADXL345 breakout boards expose a 2×8 (or 2×7) pin header with the
following signals. The table maps each breakout pin to a XIAO ESP32-C3 pin.

| Breakout pin | Function | XIAO Pin | GPIO | Notes |
|--------------|----------|----------|------|-------|
| VCC  | 3.3 V power  | 3V3      | —    | 3.3 V power |
| GND  | Ground       | GND      | —    | Ground |
| SDA  | I2C data     | D4       | GPIO6 | Data |
| SCL  | I2C clock    | D5       | GPIO7 | Clock |
| SDO  | SPI data out | —        | —    | Not used in I2C mode |
| CS   | SPI chip select | —      | —    | Not used in I2C mode |
| INT1 | Interrupt 1  | —        | —    | Optional (not used by this driver) |
| INT2 | Interrupt 2  | —        | —    | Optional (not used by this driver) |

> Some breakouts label the SPI pins `SDO`/`CS` or `MOSI`/`MISO` — only the
> I2C pair (`SDA`/`SCL`) is needed here. If your breakout has a `GND` pin
> in the middle of the header, connect it to the XIAO GND as well.

### Wiring diagram

```
  XIAO ESP32-C3              ADXL345 breakout
  ─────────────              ────────────────
  3V3  ──────────────────►  VCC
  GND  ──────────────────►  GND
  D4   ──────────────────►  SDA
  D5   ──────────────────►  SCL
```

> **No extra parts needed.** Unlike a bare ADXL345 chip, a breakout board
> already has the power regulator and decoupling capacitors fitted, so you only
> need the four wires above.

## Installation

Add this to your ESPHome configuration to pull the component from this
repository on GitHub:

```yaml
external_components:
  - source: github://eddietheengineer/esphome-adxl345
    components: [adxl345]
```

ESPHome clones the repo and loads the `adxl345` component automatically. No
local files are required.

<details>
<summary><b>Alternative: local folder (for development / forking)</b></summary>

If you fork the repository or want to develop against it locally, clone it next
to your ESPHome YAML file and reference the local `components/` folder instead:

```yaml
external_components:
  - source:
      type: local
      path: ./components
```

</details>

## Example ESPHome configuration

```yaml
esphome:
  name: adxl345-xiao-c3

external_components:
  - source: github://eddietheengineer/esphome-adxl345
    components: [adxl345]

esp32:
  board: esp32-c3-devkitm-1
  variant: esp32c3
  framework:
    type: arduino

logger:

# I2C bus.
i2c:
  sda: GPIO6    # XIAO D4 → SDA
  scl: GPIO7    # XIAO D5 → SCL
  frequency: 400 kHz

# ADXL345 component (fixed I2C address 0x53).
adxl345:
  id: adxl345_sensor
  name: ADXL345
  output_rate: 100 Hz   # Sensor output data rate (BW_RATE register)
  range: 4 g            # Full-scale range (±2/±4/±8/±16 g)
  full_resolution: true # 4 mg/LSB in all ranges (vs 10-bit fixed)
  low_power: false
  # Vibration analysis: FFT of the Y axis (requires 400 kHz I2C).
  vibration:
    axis: y
    sample_rate: 1000
    window: 2s
    min_frequency: 1

# Acceleration sensors (in g).
sensor:
  - platform: adxl345
    name: "ADXL345 X Acceleration"
    axis: x
    adxl345_id: adxl345_sensor
    id: accel_x

  - platform: adxl345
    name: "ADXL345 Y Acceleration"
    axis: y
    adxl345_id: adxl345_sensor
    id: accel_y

  - platform: adxl345
    name: "ADXL345 Z Acceleration"
    axis: z
    adxl345_id: adxl345_sensor
    id: accel_z

  - platform: adxl345
    name: "ADXL345 Magnitude"
    axis: magnitude
    adxl345_id: adxl345_sensor
    id: accel_mag

  # Tilt angles (in degrees), derived from the static gravity vector.
  - platform: adxl345
    name: "ADXL345 Tilt X"
    axis: tilt_x
    adxl345_id: adxl345_sensor
    id: tilt_x

  - platform: adxl345
    name: "ADXL345 Tilt Y"
    axis: tilt_y
    adxl345_id: adxl345_sensor
    id: tilt_y

  - platform: adxl345
    name: "ADXL345 Tilt Z"
    axis: tilt_z
    adxl345_id: adxl345_sensor
    id: tilt_z

  # Vibration analysis (FFT of the Y axis).
  - platform: adxl345
    name: "ADXL345 Vibration Frequency"
    axis: vibration_frequency
    adxl345_id: adxl345_sensor
    id: vib_freq

  - platform: adxl345
    name: "ADXL345 Vibration Amplitude"
    axis: vibration_amplitude
    adxl345_id: adxl345_sensor
    id: vib_amp

  - platform: adxl345
    name: "ADXL345 Vibration Deflection"
    axis: vibration_deflection
    adxl345_id: adxl345_sensor
    id: vib_defl

# Event / interrupt binary sensors.
#
# To enable tap / activity / inactivity / free-fall detection, set the
# corresponding threshold registers on the adxl345 component, e.g.:
#
#   adxl345:
#     ...
#     threshold_tap: 40         # ~2.5 g tap threshold
#     tap_duration: 200        # ~125 ms
#     tap_latency: 50          # ~62 ms
#     tap_window: 200         # ~250 ms
#     threshold_activity: 30
#     threshold_free_fall: 7   # ~440 mg
#     time_free_fall: 20       # ~100 ms
#     int_enable: 0xFF         # enable all interrupt sources
#
binary_sensor:
  - platform: adxl345
    name: "ADXL345 Data Ready"
    source: data_ready
    adxl345_id: adxl345_sensor

  - platform: adxl345
    name: "ADXL345 Single Tap"
    source: single_tap
    adxl345_id: adxl345_sensor

  - platform: adxl345
    name: "ADXL345 Double Tap"
    source: double_tap
    adxl345_id: adxl345_sensor

  - platform: adxl345
    name: "ADXL345 Activity"
    source: activity
    adxl345_id: adxl345_sensor

  - platform: adxl345
    name: "ADXL345 Inactivity"
    source: inactivity
    adxl345_id: adxl345_sensor

  - platform: adxl345
    name: "ADXL345 Free Fall"
    source: free_fall
    adxl345_id: adxl345_sensor
```

## Configuration reference

### `adxl345` component

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `name` | string | — | Component name (required). |
| `i2c_id` | id | first bus | Reference to the `i2c:` bus. |
| `address` | int | `0x53` | I2C address. Fixed at 0x53 by the ADXL345 hardware. |
| `output_rate` | enum | `100 Hz` | Sensor output data rate (BW_RATE register). Options: `6.25 Hz` … `3200 Hz`. |
| `range` | enum | `4 g` | Full-scale range. Options: `2 g`, `4 g`, `8 g`, `16 g`. |
| `full_resolution` | bool | `true` | Full-resolution mode (4 mg/LSB in all ranges) vs 10-bit fixed. |
| `low_power` | bool | `false` | Reduced-power mode (slightly higher noise). |
| `self_test` | bool | `false` | Enable self-test force on startup. |
| `int_invert` | bool | `false` | Invert interrupt polarity. |
| `justify` | bool | `false` | Left-justify data output. |
| `wakeup` | enum | `8 Hz` | Sleep-mode wakeup rate. Options: `8 Hz`, `4 Hz`, `2 Hz`, `1 Hz`. |
| `auto_sleep` | bool | `false` | Auto-switch to sleep on inactivity. |
| `link` | bool | `false` | Link activity/inactivity functions. |
| `sleep` | bool | `false` | Start in sleep mode. |
| `fifo_mode` | enum | `bypass` | FIFO mode: `bypass`, `fifo`, `stream`, `trigger`. |
| `fifo_samples` | int | `0` | FIFO watermark level (0–31). |
| `fifo_trigger` | bool | `false` | FIFO trigger mode. |
| `threshold_tap` | int | `0` | Tap threshold (62.5 mg/LSB). |
| `threshold_activity` | int | `0` | Activity threshold (62.5 mg/LSB). |
| `threshold_inactivity` | int | `0` | Inactivity threshold (62.5 mg/LSB). |
| `threshold_free_fall` | int | `0` | Free-fall threshold (62.5 mg/LSB). |
| `time_free_fall` | int | `0` | Free-fall time (5 ms/LSB). |
| `tap_duration` | int | `0` | Tap duration (625 µs/LSB). |
| `tap_latency` | int | `0` | Tap latency (1.25 ms/LSB). |
| `tap_window` | int | `0` | Tap window (1.25 ms/LSB). |
| `time_inactivity` | int | `0` | Inactivity time (1 s/LSB). |
| `tap_axes` | int | `0` | Tap axis enable bits. |
| `act_inact_ctl` | int | `0` | Activity/inactivity axis enable bits. |
| `int_enable` | int | `0` | Interrupt enable bits. |
| `int_map` | int | `0` | Interrupt mapping bits. |
| `offset_x` | int | `0` | X-axis offset (−128…127, 15.6 mg/LSB). |
| `offset_y` | int | `0` | Y-axis offset. |
| `offset_z` | int | `0` | Z-axis offset. |

### `vibration` block

The optional `vibration:` sub-block runs an on-device FFT on one axis and
exposes the dominant frequency, its amplitude, the resulting deflection, and
the peak absolute amplitude over the window.

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `axis` | enum | `y` | Which axis to analyze (`x`, `y`, or `z`). |
| `sample_rate` | int | `1000` | Sampling rate in Hz (100–1000); sets the ODR and poll rate. |
| `window` | time period | `2s` | FFT window length (e.g. `2s`, `500ms`). |
| `min_frequency` | float | `1` | Lowest frequency to report, in Hz. |

### `sensor` platform

| Key | Type | Description |
|-----|------|-------------|
| `axis` | enum | `x`, `y`, `z`, `magnitude` (acceleration in g), `tilt_x`, `tilt_y`, `tilt_z` (tilt in degrees), or `vibration_frequency` (Hz), `vibration_amplitude` (g), `vibration_deflection` (mm), `vibration_peak` (g). |
| `adxl345_id` | id | Reference to the `adxl345` component. |

### `binary_sensor` platform

| Key | Type | Description |
|-----|------|-------------|
| `source` | enum | `data_ready`, `single_tap`, `double_tap`, `activity`, `inactivity`, `free_fall`. |
| `adxl345_id` | id | Reference to the `adxl345` component. |

## How it works

- The driver polls the ADXL345's six data registers (0x32–0x37) in a single I2C burst read on every `update_interval` (default 100 ms).
- Raw 16-bit two's-complement samples are converted to **g** using the configured range and resolution mode.
- Tilt angles are computed from the static gravity vector using `atan2`.
- The `INT_SOURCE` register (0x30) is polled each cycle; the tap, double-tap, activity, inactivity, and free-fall bits are reported as the current on/off level of the corresponding binary sensors (data-ready is reported whenever it is set).
- With `vibration:` enabled, the driver samples the chosen axis at up to 1 kHz, accumulates a window of samples, and runs a radix-2 FFT on the main loop. The strongest spectral bin (above `min_frequency`) is reported as the dominant frequency, its amplitude (in g), and the resulting deflection (in mm).
- The `vibration_peak` sensor reports the maximum absolute deviation from the mean over the same window (a time-domain peak in g) — a "how hard did it shake" number that is independent of frequency.

## Troubleshooting

- **"ADXL345 not detected"** — Verify the SDA/SCL wiring, that the breakout's VCC is connected to 3.3 V (not 5 V), and that the device answers at address 0x53 (e.g. with an I2C bus scanner). Check the serial log for the DEVID error.
- **Noisy readings** — Breakout boards already include decoupling capacitors. If you are using a bare ADXL345 chip (not a breakout), add a 1 µF capacitor at VS and a 0.1 µF at VDD I/O, and make sure the I2C bus has pull-ups (most ESP boards and breakouts already provide them).
- **Wrong tilt direction** — The ADXL345's axes depend on how the sensor is mounted. Adjust the tilt sensor `axis` values or add a rotation in a Home Assistant template sensor if needed.
- **Vibration sensors not updating** — Vibration analysis needs a 400 kHz I2C bus (`frequency: 400 kHz` in the `i2c:` block). At the default 100 kHz the 1 kHz sampling rate cannot be sustained, so the FFT window never fills.
