#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <array>

namespace esphome {
namespace cubic_co2 {

static const char *const TAG = "cubic_co2";

/// Cubic CO2-C18S NDIR sensor (active upload protocol, see README).
///
/// The sensor pushes a 7-byte frame roughly every 3 s on its own, so this
/// component never polls: every valid frame is published immediately. Rate
/// limiting, if wanted, belongs in the YAML `filters:` section (e.g.
/// `throttle: 30s`), where it composes with the other sensor filters.
class CubicCo2Sensor : public Component, public uart::UARTDevice, public sensor::Sensor {
 public:
  void dump_config() override { ESP_LOGCONFIG(TAG, "Cubic CO2 sensor:"); }

  void loop() override {
    while (this->available()) {
      uint8_t byte;
      if (!this->read_byte(&byte))
        return;

      // Resynchronise on the frame header if we are not inside a frame yet.
      if (this->pos_ == 0 && byte != FRAME_HEADER)
        continue;

      this->buffer_[this->pos_++] = byte;
      if (this->pos_ == FRAME_LENGTH) {
        this->parse_frame_();
        this->pos_ = 0;
      }
    }
  }

 protected:
  static const uint8_t FRAME_HEADER = 0x5C;
  static const uint8_t FRAME_LENGTH = 7;

  void parse_frame_() {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < FRAME_LENGTH - 1; i++)
      checksum ^= this->buffer_[i];

    if (checksum != this->buffer_[FRAME_LENGTH - 1]) {
      ESP_LOGW(TAG, "Checksum mismatch: 0x%02X != 0x%02X", checksum, this->buffer_[FRAME_LENGTH - 1]);
      return;
    }

    const uint16_t co2 = (uint16_t(this->buffer_[4]) << 8) | this->buffer_[5];
    ESP_LOGD(TAG, "CO2: %u ppm", co2);
    this->publish_state(co2);
  }

  std::array<uint8_t, FRAME_LENGTH> buffer_{};
  uint8_t pos_{0};
};

}  // namespace cubic_co2
}  // namespace esphome
