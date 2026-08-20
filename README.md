# ADXL345 ESPHome External Component

A 3-axis accelerometer driver for the [Analog Devices ADXL345](https://www.analog.com/en/products/adxl345.html), connected via **SPI (4-wire mode)**. Exposes X/Y/Z acceleration, vector magnitude, tilt angles, and interrupt events (tap, double-tap, activity, inactivity, free-fall) as Home Assistant sensors.

## Hardware

| Item | Notes |
|------|-------|
| **Sensor** | ADXL345 (LGA-14 package) or breakout board |
| **MCU** | Any ESP32 / ESP8266 / RP2040 with SPI. Example below uses the **Seeed XIAO ESP32-C3**. |
| **Supply** | 3.3 V (the ADXL345 operates at 2.0–3.6 V; 3.3 V is standard for ESP boards) |

> **Note:** The ADXL345 requires **SPI Mode 3** (CPOL = 1, CPHA = 1) and a maximum clock of **5 MHz**. Set `spi_mode: MODE3` and `data_rate: 5 MHz` (or lower) on the SPI bus.

## Wiring — XIAO ESP32-C3 → ADXL345

The XIAO ESP32-C3 has 14 edge pins. The table below maps each ADXL345 pin to a XIAO pin.

| ADXL345 Pin | Function | XIAO Pin | GPIO | Notes |
|-------------|----------|----------|------|-------|
| 1  | VDD I/O  | 3V3      | —    | 3.3 V power |
| 2  | GND      | GND      | —    | Ground |
| 6  | VS       | 3V3      | —    | 3.3 V supply (can share with VDD I/O) |
| 7  | CS       | D1       | GPIO3 | Chip select (active low) |
| 8  | INT1     | —        | —    | Optional: interrupt output 1 |
| 9  | INT2     | —        | —    | Optional: interrupt output 2 |
| 12 | SDO/ALT  | D9       | GPIO9 | MISO (data out from sensor) |
| 13 | SDA/SDI  | D10      | GPIO10 | MOSI (data in to sensor) |
| 14 | SCL/SCLK | D8       | GPIO8 | SCLK (clock) |

### Wiring diagram

```
  XIAO ESP32-C3              ADXL345
  ─────────────              ───────
  3V3  ──────────────────►  VDD I/O (pin 1)
  3V3  ──────────────────►  VS        (pin 6)
  GND  ──────────────────►  GND       (pins 2, 4, 5)
  D1   ──────────────────►  CS        (pin 7)
  D8   ──────────────────►  SCLK      (pin 14)
  D9   ──────────────────►  SDO/MISO  (pin 12)
  D10  ──────────────────►  SDI/MOSI  (pin 13)
  (optional) D2 ────────►  INT1      (pin 8)
  (optional) D3 ────────►  INT2      (pin 9)
```

> **Decoupling:** Place a 1 µF tantalum capacitor at VS and a 0.1 µF ceramic capacitor at VDD I/O, close to the ADXL345, to reduce noise.

## Installation

### Option A: Local folder (for development)

Place this repository's `components/` folder next to your ESPHome YAML file, then add:

```yaml
external_components:
  - source:
      type: local
      path: ./components
```

### Option B: Git repository (for production)

Push this repository to GitHub, then reference it:

```yaml
external_components:
  - source: github://<your-username>/esphome-adxl345
    components: [adxl345]
```

## Example ESPHome configuration

```yaml
esphome:
  name: adxl345-xiao-c3

external_components:
  - source:
      type: local
      path: ./components

esp32:
  board: esp32-c3-devkitm-1
  variant: esp32c3
  framework:
    type: arduino

logger:

# SPI bus — the ADXL345 requires MODE3 and ≤ 5 MHz.
spi:
  id: spi_bus
  clk_pin: GPIO8      # XIAO D8  → SCLK
  mosi_pin: GPIO10    # XIAO D10 → MOSI
  miso_pin: GPIO9     # XIAO D9  → MISO

# ADXL345 component.
adxl345:
  id: adxl345_sensor
  name: ADXL345
  cs_pin: GPIO3         # XIAO D1 → CS
  spi_id: spi_bus
  spi_mode: MODE3       # Required: CPOL=1, CPHA=1
  data_rate: 5 MHz      # SPI bus clock (max 5 MHz)
  output_rate: 100 Hz   # Sensor output data rate (BW_RATE register)
  range: 4 g            # Full-scale range (±2/±4/±8/±16 g)
  full_resolution: true # 4 mg/LSB in all ranges (vs 10-bit fixed)
  low_power: false

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
| `cs_pin` | pin | — | Chip-select pin (required). |
| `spi_id` | id | — | Reference to the `spi:` bus (required). |
| `spi_mode` | enum | `MODE3` | SPI mode. Must be `MODE3` for the ADXL345. |
| `data_rate` | frequency | `5 MHz` | SPI bus clock. Max 5 MHz. |
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

### `sensor` platform

| Key | Type | Description |
|-----|------|-------------|
| `axis` | enum | `x`, `y`, `z`, `magnitude` (acceleration in g) or `tilt_x`, `tilt_y`, `tilt_z` (tilt in degrees). |
| `adxl345_id` | id | Reference to the `adxl345` component. |

### `binary_sensor` platform

| Key | Type | Description |
|-----|------|-------------|
| `source` | enum | `data_ready`, `single_tap`, `double_tap`, `activity`, `inactivity`, `free_fall`. |
| `adxl345_id` | id | Reference to the `adxl345` component. |

## How it works

- The driver polls the ADXL345's six data registers (0x32–0x37) in a single SPI burst read on every `update_interval` (default 100 ms).
- Raw 16-bit two's-complement samples are converted to **g** using the configured range and resolution mode.
- Tilt angles are computed from the static gravity vector using `atan2`.
- The `INT_SOURCE` register (0x30) is polled each cycle; edge-triggered events (tap, double-tap, activity, inactivity, free-fall) fire the corresponding binary sensor callbacks.

## Troubleshooting

- **"ADXL345 not detected"** — Verify the CS pin, SPI mode (must be `MODE3`), and that the sensor is powered. Check the serial log for the DEVID error.
- **Noisy readings** — Add decoupling capacitors (1 µF at VS, 0.1 µF at VDD I/O). Try lowering `data_rate` to `2 MHz` or `1 MHz`.
- **Wrong tilt direction** — The ADXL345's axes depend on how the sensor is mounted. Adjust the tilt sensor `axis` values or add a rotation in a Home Assistant template sensor if needed.
