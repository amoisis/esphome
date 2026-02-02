#pragma once

#if defined(USE_ESP32_VARIANT_ESP32P4) || defined(USE_ESP32_VARIANT_ESP32S2) || defined(USE_ESP32_VARIANT_ESP32S3)

#include "esphome/core/component.h"
#include <cstdint>
#include <cstring>

namespace esphome::hid_keyboard {

class HIDKeyboard : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Send a key with modifier
  void send_key(uint8_t keycode, uint8_t modifier = 0);

  // Release all keys
  void release_all();

  // Send key and release it (non-blocking)
  void send_keypress(uint8_t keycode, uint8_t modifier = 0);

 protected:
  // HID keyboard report buffer
  uint8_t keyboard_report_[8]{};

  // Last keypress time for auto-release
  uint32_t last_keypress_time_{0};
  bool keypress_pending_{false};

  // Key release delay in milliseconds
  static constexpr uint32_t KEY_RELEASE_DELAY_MS = 50;

  // Initialize TinyUSB HID
  void init_hid_();

  // Send raw HID report
  void send_report_();
};

}  // namespace esphome::hid_keyboard

#endif  // USE_ESP32_VARIANT_ESP32P4 || USE_ESP32_VARIANT_ESP32S2 || USE_ESP32_VARIANT_ESP32S3
