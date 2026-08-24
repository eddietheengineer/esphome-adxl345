#include "adxl345.h"
#include "esphome/core/log.h"

namespace esphome {
namespace adxl345 {

static const char *TAG = "adxl345";

// ---------------------------------------------------------------------------
// Low-level I2C register access.
//
// The ADXL345 uses a register-pointer protocol over I2C (fixed 7-bit
// address 0x53): the controller writes the 6-bit register address, then
// either reads data bytes or writes a data byte. On reads the register
// pointer auto-increments, so a burst read of the six data registers is a
// single 6-byte transfer starting at 0x32.
// ---------------------------------------------------------------------------
bool ADXL345::read_register(uint8_t reg, uint8_t *out) {
  return this->read_byte(reg, out);
}

bool ADXL345::write_register(uint8_t reg, uint8_t value) {
  return this->write_byte(reg, value);
}

bool ADXL345::read_data_registers(uint8_t *out) {
  // Burst read of the six data registers (0x32..0x37) in one transaction;
  // the register pointer auto-increments.
  return this->read_bytes(REG_DATA_X0, out, 6);
}

bool ADXL345::read_fifo_status(uint8_t *out) {
  return this->read_register(REG_FIFO_STATUS, out);
}

bool ADXL345::read_int_source(uint8_t *out) {
  return this->read_register(REG_INT_SOURCE, out);
}

// ---------------------------------------------------------------------------
// Data conversion.
// ---------------------------------------------------------------------------
float ADXL345::full_scale_g() const {
  switch (this->range_bits_) {
    case 0x00:
      return 2.0f;
    case 0x01:
      return 4.0f;
    case 0x02:
      return 8.0f;
    case 0x03:
    default:
      return 16.0f;
  }
}

float ADXL345::raw_to_g(int16_t raw) const {
  if (this->full_res_) {
    // Full-resolution mode: 4 mg/LSB scale factor in every g range.
    return raw * 0.004f;
  }
  // 10-bit mode: the 10-bit value is left-justified in the 16-bit word.
  // Reconstruct the 10-bit signed value, then scale by the range.
  int16_t ten_bit = raw >> 6;
  // ten_bit is now a 10-bit two's-complement value sign-extended to 16 bits.
  float scale = this->full_scale_g() / 512.0f;  // 2^9 LSBs per full scale
  return ten_bit * scale;
}

// ---------------------------------------------------------------------------
// Setup.
// ---------------------------------------------------------------------------
void ADXL345::setup() {
  // Verify the device is present and is an ADXL345.
  uint8_t devid = 0;
  if (!this->read_register(REG_DEVID, &devid) || devid != DEVID_VALUE) {
    ESP_LOGE(TAG, "ADXL345 not detected (DEVID=0x%02X, expected 0x%02X). Check the I2C wiring (SDA/SCL) and that the device answers at address 0x53.",
             devid, DEVID_VALUE);
    this->setup_failed_ = true;
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "ADXL345 detected (DEVID=0x%02X)", devid);

  // Configure the data format register.
  uint8_t data_format = 0x00;
  if (this->full_res_)
    data_format |= DF_FULL_RES;
  if (this->justify_)
    data_format |= DF_JUSTIFY;
  if (this->int_invert_)
    data_format |= DF_INT_INVERT;
  if (this->self_test_)
    data_format |= DF_SELF_TEST;
  // I2C mode: the DF_SPI bit (bit 6) must be 0.
  data_format &= ~DF_SPI;
  data_format |= (this->range_bits_ & DF_RANGE_MASK);
  this->write_register(REG_DATA_FORMAT, data_format);

  // Configure the bandwidth / data rate register.
  uint8_t bw_rate = (this->rate_code_ & BR_RATE_MASK);
  if (this->low_power_)
    bw_rate |= BR_LOW_POWER;
  this->write_register(REG_BW_RATE, bw_rate);

  // Configure the power control register.
  uint8_t power_ctl = 0x00;
  if (this->link_)
    power_ctl |= PC_LINK;
  if (this->auto_sleep_)
    power_ctl |= PC_AUTO_SLEEP;
  if (this->sleep_)
    power_ctl |= PC_SLEEP;
  power_ctl |= (this->wakeup_bits_ & PC_WAKEUP_MASK);
  // Start in measurement mode.
  power_ctl |= PC_MEASURE;
  this->write_register(REG_POWER_CTL, power_ctl);

  // Configure the FIFO control register.
  uint8_t fifo_ctl = (this->fifo_mode_ & 0x03) << 6;
  if (this->fifo_trigger_)
    fifo_ctl |= 0x10;
  fifo_ctl |= (this->fifo_samples_ & 0x1F);
  this->write_register(REG_FIFO_CTL, fifo_ctl);

  // Configure the interrupt enable / map registers.
  this->write_register(REG_INT_ENABLE, this->int_enable_);
  this->write_register(REG_INT_MAP, this->int_map_);

  this->write_register(REG_THRESH_ACT, this->thresh_act_);
  this->write_register(REG_THRESH_INACT, this->thresh_inact_);
  this->write_register(REG_THRESH_FF, this->thresh_ff_);
  this->write_register(REG_TIME_FF, this->time_ff_);
  this->write_register(REG_DUR, this->dur_);
  this->write_register(REG_LATENT, this->latent_);
  this->write_register(REG_WINDOW, this->window_);
  this->write_register(REG_TIME_INACT, this->time_inact_);
  this->write_register(REG_TAP_AXES, this->tap_axes_);
  this->write_register(REG_ACT_INACT_CTL, this->act_inact_ctl_);

  // Configure the per-axis offset registers.
  this->write_register(REG_OFSX, static_cast<uint8_t>(this->ofsx_));
  this->write_register(REG_OFSY, static_cast<uint8_t>(this->ofsy_));
  this->write_register(REG_OFSZ, static_cast<uint8_t>(this->ofsz_));

  // Prime the interrupt source state so the first loop() iteration does not
  // fire a spurious edge.
  this->read_int_source(&this->last_int_source_);

  ESP_LOGD(TAG, "ADXL345 configured: range=%+dg, full_res=%d, rate_code=0x%02X, low_power=%d",
           this->full_scale_g(), this->full_res_, this->rate_code_, this->low_power_);
}

void ADXL345::dump_config() {
  ESP_LOGCONFIG(TAG, "ADXL345");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Range: %+.0f g", this->full_scale_g());
  ESP_LOGCONFIG(TAG, "  Full resolution: %s", this->full_res_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Data rate code: 0x%02X", this->rate_code_);
  ESP_LOGCONFIG(TAG, "  Low power: %s", this->low_power_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Self test: %s", this->self_test_ ? "yes" : "no");
}

// ---------------------------------------------------------------------------
// Main polling update.
// ---------------------------------------------------------------------------
void ADXL345::update() {
  if (this->setup_failed_)
    return;

  // Read the six data registers in a single burst.
  uint8_t raw[6];
  if (!this->read_data_registers(raw))
    return;

  // Reassemble the 16-bit two's-complement samples (MSB first on the wire).
  int16_t x = static_cast<int16_t>((static_cast<uint16_t>(raw[1]) << 8) | raw[0]);
  int16_t y = static_cast<int16_t>((static_cast<uint16_t>(raw[3]) << 8) | raw[2]);
  int16_t z = static_cast<int16_t>((static_cast<uint16_t>(raw[5]) << 8) | raw[4]);

  this->x_g_ = this->raw_to_g(x);
  this->y_g_ = this->raw_to_g(y);
  this->z_g_ = this->raw_to_g(z);

  // Fire the acceleration callbacks.
  if (this->x_callback_)
    this->x_callback_(this->x_g_);
  if (this->y_callback_)
    this->y_callback_(this->y_g_);
  if (this->z_callback_)
    this->z_callback_(this->z_g_);

  // Magnitude (RSS) of the acceleration vector.
  float mag = std::sqrt(this->x_g_ * this->x_g_ + this->y_g_ * this->y_g_ + this->z_g_ * this->z_g_);
  if (this->magnitude_callback_)
    this->magnitude_callback_(mag);

  // Tilt angles (degrees) derived from the static gravity vector.
  // atan2 convention: angle between the axis and the +Z (up) axis.
  float tilt_x = std::atan2(this->y_g_, this->z_g_) * 180.0f / static_cast<float>(M_PI);
  float tilt_y = std::atan2(-this->x_g_, this->z_g_) * 180.0f / static_cast<float>(M_PI);
  float tilt_z = std::atan2(this->x_g_, this->y_g_) * 180.0f / static_cast<float>(M_PI);
  if (this->tilt_x_callback_)
    this->tilt_x_callback_(tilt_x);
  if (this->tilt_y_callback_)
    this->tilt_y_callback_(tilt_y);
  if (this->tilt_z_callback_)
    this->tilt_z_callback_(tilt_z);

  // Read the interrupt source register and fire edge-triggered callbacks.
  uint8_t int_source = 0;
  if (this->read_int_source(&int_source)) {
    // DATA_READY is level-based and is cleared by reading the data registers,
    // so we report it every time it is set.
    if (int_source & IS_DATA_READY) {
      if (this->data_ready_callback_)
        this->data_ready_callback_();
    }
    // The remaining interrupts are edge-triggered: fire only on a rising
    // edge relative to the previous read.
    uint8_t rising = int_source & ~this->last_int_source_;
    if (rising & IS_SINGLE_TAP) {
      if (this->single_tap_callback_)
        this->single_tap_callback_();
    }
    if (rising & IS_DOUBLE_TAP) {
      if (this->double_tap_callback_)
        this->double_tap_callback_();
    }
    if (rising & IS_ACTIVITY) {
      if (this->activity_callback_)
        this->activity_callback_();
    }
    if (rising & IS_INACTIVITY) {
      if (this->inactivity_callback_)
        this->inactivity_callback_();
    }
    if (rising & IS_FREE_FALL) {
      if (this->free_fall_callback_)
        this->free_fall_callback_();
    }
    this->last_int_source_ = int_source;
  }

  // Report the FIFO status (number of buffered samples).
  uint8_t fifo_status = 0;
  if (this->read_fifo_status(&fifo_status)) {
    if (this->fifo_status_callback_)
      this->fifo_status_callback_(fifo_status & 0x1F);
  }
}

// ---------------------------------------------------------------------------
// Self-test.
// ---------------------------------------------------------------------------
bool ADXL345::run_self_test() {
  // Read the current data format register, enable self-test, wait for the
  // output to settle (4 time constants), then disable it again.
  uint8_t df = 0;
  if (!this->read_register(REG_DATA_FORMAT, &df))
    return false;

  this->write_register(REG_DATA_FORMAT, df | DF_SELF_TEST);
  // At 100 Hz the time constant is 10 ms, so 4*tau = 40 ms. Use 100 ms to
  // be safe across data rates.
  delay(100);

  // Read the shifted output.
  uint8_t raw[6];
  if (!this->read_data_registers(raw))
    return false;
  int16_t x = static_cast<int16_t>((static_cast<uint16_t>(raw[1]) << 8) | raw[0]);
  int16_t y = static_cast<int16_t>((static_cast<uint16_t>(raw[3]) << 8) | raw[2]);
  int16_t z = static_cast<int16_t>((static_cast<uint16_t>(raw[5]) << 8) | raw[4]);

  // Disable self-test.
  this->write_register(REG_DATA_FORMAT, df & ~DF_SELF_TEST);

  // The self-test should produce a measurable shift on at least one axis.
  float shift = std::max({std::abs(this->raw_to_g(x)), std::abs(this->raw_to_g(y)),
                           std::abs(this->raw_to_g(z))});
  ESP_LOGD(TAG, "Self-test shift: %.3f g", shift);
  return shift > 0.05f;  // a non-trivial shift indicates a healthy sensor
}

}  // namespace adxl345
}  // namespace esphome
