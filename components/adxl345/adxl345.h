#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace adxl345 {

// ---------------------------------------------------------------------------
// ADXL345 register map (Analog Devices ADXL345 datasheet, Rev. 0)
// ---------------------------------------------------------------------------
enum Register : uint8_t {
  REG_DEVID = 0x00,
  REG_THRESH_TAP = 0x1D,
  REG_OFSX = 0x1E,
  REG_OFSY = 0x1F,
  REG_OFSZ = 0x20,
  REG_DUR = 0x21,
  REG_LATENT = 0x22,
  REG_WINDOW = 0x23,
  REG_THRESH_ACT = 0x24,
  REG_THRESH_INACT = 0x25,
  REG_TIME_INACT = 0x26,
  REG_ACT_INACT_CTL = 0x27,
  REG_THRESH_FF = 0x28,
  REG_TIME_FF = 0x29,
  REG_TAP_AXES = 0x2A,
  REG_ACT_TAP_STATUS = 0x2B,
  REG_BW_RATE = 0x2C,
  REG_POWER_CTL = 0x2D,
  REG_INT_ENABLE = 0x2E,
  REG_INT_MAP = 0x2F,
  REG_INT_SOURCE = 0x30,
  REG_DATA_FORMAT = 0x31,
  REG_DATA_X0 = 0x32,
  REG_DATA_X1 = 0x33,
  REG_DATA_Y0 = 0x34,
  REG_DATA_Y1 = 0x35,
  REG_DATA_Z0 = 0x36,
  REG_DATA_Z1 = 0x37,
  REG_FIFO_CTL = 0x38,
  REG_FIFO_STATUS = 0x39,
};

// Fixed device ID reported by the ADXL345 in register 0x00.
constexpr uint8_t DEVID_VALUE = 0xE5;

// DATA_FORMAT register (0x31) bit fields.
constexpr uint8_t DF_SELF_TEST = 0x80;
constexpr uint8_t DF_SPI = 0x40;
constexpr uint8_t DF_INT_INVERT = 0x20;
constexpr uint8_t DF_FULL_RES = 0x08;
constexpr uint8_t DF_JUSTIFY = 0x04;
constexpr uint8_t DF_RANGE_MASK = 0x03;

// POWER_CTL register (0x2D) bit fields.
constexpr uint8_t PC_LINK = 0x40;
constexpr uint8_t PC_AUTO_SLEEP = 0x20;
constexpr uint8_t PC_MEASURE = 0x08;
constexpr uint8_t PC_SLEEP = 0x04;
constexpr uint8_t PC_WAKEUP_MASK = 0x03;

// BW_RATE register (0x2C) bit fields.
constexpr uint8_t BR_LOW_POWER = 0x10;
constexpr uint8_t BR_RATE_MASK = 0x0F;

// INT_SOURCE register (0x30) bit fields.
constexpr uint8_t IS_DATA_READY = 0x80;
constexpr uint8_t IS_SINGLE_TAP = 0x40;
constexpr uint8_t IS_DOUBLE_TAP = 0x20;
constexpr uint8_t IS_ACTIVITY = 0x10;
constexpr uint8_t IS_INACTIVITY = 0x08;
constexpr uint8_t IS_FREE_FALL = 0x04;
constexpr uint8_t IS_WATERMARK = 0x02;
constexpr uint8_t IS_OVERRUN = 0x01;

// g-range setting (DATA_FORMAT range bits) -> full-scale acceleration in g.
struct RangeSetting {
  uint8_t bits;
  float full_scale_g;
};

// ---------------------------------------------------------------------------
// ADXL345 driver.
//
// The device is driven over I2C at its fixed 7-bit address 0x53. Register
// access uses the standard register-pointer protocol: write the register
// address, then read (the pointer auto-increments for burst reads) or write
// the data byte. The ADXL345's SDO and CS pins are SPI-only and are not used
// in I2C mode.
// ---------------------------------------------------------------------------
class ADXL345 : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_range(uint8_t range_bits) { this->range_bits_ = range_bits; }
  void set_data_rate_code(uint8_t rate_code) { this->rate_code_ = rate_code; }
  void set_low_power(bool low_power) { this->low_power_ = low_power; }
  void set_full_resolution(bool full_res) { this->full_res_ = full_res; }
  void set_self_test(bool self_test) { this->self_test_ = self_test; }
  void set_int_invert(bool invert) { this->int_invert_ = invert; }
  void set_justify(bool justify) { this->justify_ = justify; }
  void set_wakeup_bits(uint8_t wakeup) { this->wakeup_bits_ = wakeup; }
  void set_auto_sleep(bool auto_sleep) { this->auto_sleep_ = auto_sleep; }
  void set_link(bool link) { this->link_ = link; }
  void set_sleep(bool sleep) { this->sleep_ = sleep; }
  void set_fifo_mode(uint8_t mode) { this->fifo_mode_ = mode; }
  void set_fifo_samples(uint8_t samples) { this->fifo_samples_ = samples; }
  void set_fifo_trigger(bool trigger) { this->fifo_trigger_ = trigger; }
  void set_thresh_tap(uint8_t v) { this->thresh_tap_ = v; }
  void set_thresh_act(uint8_t v) { this->thresh_act_ = v; }
  void set_thresh_inact(uint8_t v) { this->thresh_inact_ = v; }
  void set_thresh_ff(uint8_t v) { this->thresh_ff_ = v; }
  void set_time_ff(uint8_t v) { this->time_ff_ = v; }
  void set_dur(uint8_t v) { this->dur_ = v; }
  void set_latent(uint8_t v) { this->latent_ = v; }
  void set_window(uint8_t v) { this->window_ = v; }
  void set_time_inact(uint8_t v) { this->time_inact_ = v; }
  void set_tap_axes(uint8_t v) { this->tap_axes_ = v; }
  void set_act_inact_ctl(uint8_t v) { this->act_inact_ctl_ = v; }
  void set_int_enable(uint8_t v) { this->int_enable_ = v; }
  void set_int_map(uint8_t v) { this->int_map_ = v; }
  void set_offset_x(int8_t v) { this->ofsx_ = v; }
  void set_offset_y(int8_t v) { this->ofsy_ = v; }
  void set_offset_z(int8_t v) { this->ofsz_ = v; }

  // Sensor callbacks (wired up by the Python sensor platforms).
  void set_x_callback(std::function<void(float)> &&f) { this->x_callback_ = std::move(f); }
  void set_y_callback(std::function<void(float)> &&f) { this->y_callback_ = std::move(f); }
  void set_z_callback(std::function<void(float)> &&f) { this->z_callback_ = std::move(f); }
  void set_magnitude_callback(std::function<void(float)> &&f) { this->magnitude_callback_ = std::move(f); }
  void set_tilt_x_callback(std::function<void(float)> &&f) { this->tilt_x_callback_ = std::move(f); }
  void set_tilt_y_callback(std::function<void(float)> &&f) { this->tilt_y_callback_ = std::move(f); }
  void set_tilt_z_callback(std::function<void(float)> &&f) { this->tilt_z_callback_ = std::move(f); }
  void set_data_ready_callback(std::function<void()> &&f) { this->data_ready_callback_ = std::move(f); }
  void set_single_tap_callback(std::function<void(bool)> &&f) { this->single_tap_callback_ = std::move(f); }
  void set_double_tap_callback(std::function<void(bool)> &&f) { this->double_tap_callback_ = std::move(f); }
  void set_activity_callback(std::function<void(bool)> &&f) { this->activity_callback_ = std::move(f); }
  void set_inactivity_callback(std::function<void(bool)> &&f) { this->inactivity_callback_ = std::move(f); }
  void set_free_fall_callback(std::function<void(bool)> &&f) { this->free_fall_callback_ = std::move(f); }
  void set_fifo_status_callback(std::function<void(uint8_t)> &&f) { this->fifo_status_callback_ = std::move(f); }

  // Trigger a self-test pulse: enables self-test, waits for the output to
  // settle, then disables it again. Returns true if the sensor responded.
  bool run_self_test();

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:

  void setup() override;
  void dump_config() override;
  void update() override;

  // Low-level I2C register access.
  bool read_register(uint8_t reg, uint8_t *out);
  bool write_register(uint8_t reg, uint8_t value);
  bool read_data_registers(uint8_t *out);  // 6 bytes: X0 X1 Y0 Y1 Z0 Z1
  bool read_fifo_status(uint8_t *out);
  bool read_int_source(uint8_t *out);

  // Convert a raw 16-bit two's-complement sample to g.
  float raw_to_g(int16_t raw) const;
  // Current full-scale range in g, derived from the configured range bits.
  float full_scale_g() const;

  bool setup_failed_{false};
  uint8_t range_bits_{0x01};   // default ±4 g
  uint8_t rate_code_{0x0A};    // default 100 Hz
  bool low_power_{false};
  bool full_res_{true};
  bool self_test_{false};
  bool int_invert_{false};
  bool justify_{false};
  uint8_t wakeup_bits_{0x00};
  bool auto_sleep_{false};
  bool link_{false};
  bool sleep_{false};
  uint8_t fifo_mode_{0x00};
  uint8_t fifo_samples_{0x00};
  bool fifo_trigger_{false};
  uint8_t thresh_tap_{0x00};
  uint8_t thresh_act_{0x00};
  uint8_t thresh_inact_{0x00};
  uint8_t thresh_ff_{0x00};
  uint8_t time_ff_{0x00};
  uint8_t dur_{0x00};
  uint8_t latent_{0x00};
  uint8_t window_{0x00};
  uint8_t time_inact_{0x00};
  uint8_t tap_axes_{0x00};
  uint8_t act_inact_ctl_{0x00};
  uint8_t int_enable_{0x00};
  uint8_t int_map_{0x00};
  int8_t ofsx_{0};
  int8_t ofsy_{0};
  int8_t ofsz_{0};

  // Cached latest sample (g).
  float x_g_{0.0f};
  float y_g_{0.0f};
  float z_g_{0.0f};

  // Callbacks.
  std::function<void(float)> x_callback_;
  std::function<void(float)> y_callback_;
  std::function<void(float)> z_callback_;
  std::function<void(float)> magnitude_callback_;
  std::function<void(float)> tilt_x_callback_;
  std::function<void(float)> tilt_y_callback_;
  std::function<void(float)> tilt_z_callback_;
  std::function<void()> data_ready_callback_;
  std::function<void(bool)> single_tap_callback_;
  std::function<void(bool)> double_tap_callback_;
  std::function<void(bool)> activity_callback_;
  std::function<void(bool)> inactivity_callback_;
  std::function<void(bool)> free_fall_callback_;
  std::function<void(uint8_t)> fifo_status_callback_;
};

}  // namespace adxl345
}  // namespace esphome
