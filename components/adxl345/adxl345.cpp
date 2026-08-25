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
  // Verify the device is present and is an ADXL345. The first read after
  // boot can race the sensor's power-up or the I2C bus settling (the bus
  // scan's address probe can succeed while the first data read fails), so
  // retry a few times before declaring failure.
  uint8_t devid = 0;
  bool detected = false;
  for (int attempt = 0; attempt < 5 && !detected; attempt++) {
    if (attempt > 0)
      delay(100);
    detected = this->read_register(REG_DEVID, &devid) && devid == DEVID_VALUE;
  }
  if (!detected) {
    ESP_LOGE(TAG, "ADXL345 not detected (DEVID=0x%02X, expected 0x%02X). Check the I2C wiring (SDA/SCL) and that the device answers at address 0x53.",
             devid, DEVID_VALUE);
    this->setup_failed_ = true;
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "ADXL345 detected (DEVID=0x%02X)", devid);

  // If vibration analysis is enabled, start the sampling-window clock and the
  // slow-publish clock now.
  if (this->vibration_enabled_) {
    this->window_start_us_ = micros();
    this->last_slow_us_ = micros();
    this->vib_idx_ = 0;
    this->vib_count_ = 0;
    ESP_LOGD(TAG, "Vibration analysis: axis=%d, window=%d samples, min_freq=%.1f Hz, sample_rate=%.0f Hz",
             this->vib_axis_, this->vib_window_, this->min_frequency_, this->fs_nominal_);
  }

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

  // Fast path: append the configured axis to the FFT ring buffer. When the
  // window fills, run the FFT and publish the vibration peak.
  if (this->vibration_enabled_) {
    const float sample = (this->vib_axis_ == 0) ? this->x_g_ : (this->vib_axis_ == 1) ? this->y_g_ : this->z_g_;
    if (this->vib_count_ == 0)
      this->window_start_us_ = micros();
    this->vib_buf_[this->vib_idx_] = sample;
    this->vib_idx_ = (this->vib_idx_ + 1) % static_cast<size_t>(this->vib_window_);
    if (++this->vib_count_ == static_cast<size_t>(this->vib_window_)) {
      const uint32_t duration_us = micros() - this->window_start_us_;
      if (duration_us > 0)
        this->fs_actual_ = static_cast<float>(this->vib_window_) * 1e6f / static_cast<float>(duration_us);
      this->run_vibration_analysis();
      this->vib_count_ = 0;
    }
  }

  // Slow path: publish the regular sensors and read the interrupt / FIFO
  // registers. While vibration analysis is active the update() loop runs at
  // ~1 kHz, so this work is throttled to SLOW_PERIOD_US (10 Hz) to avoid
  // flooding Home Assistant. When vibration is off, update() already runs at
  // the (slower) configured interval, so this runs on every call.
  const uint32_t now = micros();
  if (this->vibration_enabled_ && now - this->last_slow_us_ < SLOW_PERIOD_US)
    return;
  this->last_slow_us_ = now;
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

  // Read the interrupt source register. DATA_READY is reported whenever it is
  // set; the remaining event bits are reported as their current on/off level,
  // since the ADXL345 sets and clears them in hardware. Publishing the level
  // every cycle gives the binary sensors an initial state (instead of
  // "unknown") and keeps them in sync as the bits toggle.
  uint8_t int_source = 0;
  if (this->read_int_source(&int_source)) {
    if (int_source & IS_DATA_READY) {
      if (this->data_ready_callback_)
        this->data_ready_callback_();
    }
    if (this->single_tap_callback_)
      this->single_tap_callback_((int_source & IS_SINGLE_TAP) != 0);
    if (this->double_tap_callback_)
      this->double_tap_callback_((int_source & IS_DOUBLE_TAP) != 0);
    if (this->activity_callback_)
      this->activity_callback_((int_source & IS_ACTIVITY) != 0);
    if (this->inactivity_callback_)
      this->inactivity_callback_((int_source & IS_INACTIVITY) != 0);
    if (this->free_fall_callback_)
      this->free_fall_callback_((int_source & IS_FREE_FALL) != 0);
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

// ---------------------------------------------------------------------------
// Vibration analysis.
// ---------------------------------------------------------------------------
void ADXL345::run_vibration_analysis() {
  const int n = this->vib_window_;
  if (n < 2)
    return;

  // Copy the window into the FFT buffer and remove the DC offset (the static
  // gravity / bias component) so it doesn't swamp the spectrum.
  float mean = 0.0f;
  for (int i = 0; i < n; i++)
    mean += this->vib_buf_[i];
  mean /= static_cast<float>(n);

  // Peak absolute amplitude: the maximum deviation from the mean over the
  // window (a time-domain peak, in g). Published once per window, alongside
  // the FFT results.
  float peak = 0.0f;
  for (int i = 0; i < n; i++) {
    const float d = std::abs(this->vib_buf_[i] - mean);
    if (d > peak)
      peak = d;
  }
  if (this->vib_peak_callback_)
    this->vib_peak_callback_(peak);
  for (int i = 0; i < n; i++)
    this->fft_buf_[i] = std::complex<float>(this->vib_buf_[i] - mean, 0.0f);

  // In-place FFT.
  fft_radix2(this->fft_buf_.data(), n);

  // Search for the strongest spectral bin from min_frequency up to Nyquist.
  const float fs = this->fs_actual_;
  const int min_bin = std::max(1, static_cast<int>(std::ceil(this->min_frequency_ / (fs / static_cast<float>(n)))));
  int peak_bin = -1;
  float peak_mag = 0.0f;
  for (int k = min_bin; k <= n / 2; k++) {
    const float mag = std::abs(this->fft_buf_[k]);
    if (mag > peak_mag) {
      peak_mag = mag;
      peak_bin = k;
    }
  }
  if (peak_bin < 0)
    return;

  const float freq = static_cast<float>(peak_bin) * fs / static_cast<float>(n);
  const float amp_g = 2.0f * peak_mag / static_cast<float>(n);
  // Harmonic deflection: x = a / omega^2, with a in m/s^2 and omega in rad/s.
  const float omega = 2.0f * 3.14159265f * freq;
  const float defl_mm = (omega > 0.0f) ? amp_g * 9.80665f / (omega * omega) * 1000.0f : 0.0f;

  if (this->vib_freq_callback_)
    this->vib_freq_callback_(freq);
  if (this->vib_amp_callback_)
    this->vib_amp_callback_(amp_g);
  if (this->vib_defl_callback_)
    this->vib_defl_callback_(defl_mm);

  // If the dump button was pressed, dump the raw window once and disarm.
  if (this->dump_pending_) {
    this->dump_pending_ = false;
    this->dump_vibration_window();
  }
}

void ADXL345::dump_vibration_window() {
  const int n = this->vib_window_;
  if (n < 2)
    return;
  ESP_LOGI(TAG, "### VIBRATION DUMP: %d samples @ %.1f Hz (index,g) ###", n, this->fs_actual_);
  for (int i = 0; i < n; i++) {
    ESP_LOGI(TAG, "%d,%.6f", i, this->vib_buf_[i]);
    // Yield to the logger task so the serial buffer drains between lines.
    // Without this, 1024 back-to-back ESP_LOGI calls starve the logger and
    // the dump stalls/drops after a few dozen lines.
    delay(2);
  }
  ESP_LOGI(TAG, "### END VIBRATION DUMP ###");
}

void ADXL345::fft_radix2(std::complex<float> *a, int n) {
  // Bit-reversal permutation.
  for (int i = 1, j = 0; i < n; i++) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;
    if (i < j)
      std::swap(a[i], a[j]);
  }
  // Butterfly stages.
  for (int len = 2; len <= n; len <<= 1) {
    const float ang = -2.0f * 3.14159265f / static_cast<float>(len);
    const std::complex<float> wlen(std::cos(ang), std::sin(ang));
    for (int i = 0; i < n; i += len) {
      std::complex<float> w(1.0f, 0.0f);
      for (int j = 0; j < len / 2; j++) {
        const std::complex<float> u = a[i + j];
        const std::complex<float> v = a[i + j + len / 2] * w;
        a[i + j] = u + v;
        a[i + j + len / 2] = u - v;
        w = w * wlen;
      }
    }
  }
}

}  // namespace adxl345
}  // namespace esphome
