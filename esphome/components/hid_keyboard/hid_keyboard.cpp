#if defined(USE_ESP32_VARIANT_ESP32P4) || defined(USE_ESP32_VARIANT_ESP32S2) || defined(USE_ESP32_VARIANT_ESP32S3)

#include "hid_keyboard.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome::hid_keyboard {

static const char *const TAG = "hid_keyboard";

void HIDKeyboard::setup() {
  ESP_LOGI(TAG, "Setting up HID Keyboard");
  this->init_hid_();
}

void HIDKeyboard::loop() {
  // Check if we have a pending keypress that needs to be released
  if (this->keypress_pending_) {
    uint32_t now = millis();
    if (now - this->last_keypress_time_ >= KEY_RELEASE_DELAY_MS) {
      this->release_all();
    }
  }
}

void HIDKeyboard::dump_config() { ESP_LOGCONFIG(TAG, "HID Keyboard:"); }

void HIDKeyboard::init_hid_() {
  // Initialize keyboard report buffer
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->keypress_pending_ = false;
  this->last_keypress_time_ = 0;
}

void HIDKeyboard::send_key(uint8_t keycode, uint8_t modifier) {
  // keyboard_report[0] = modifier keys
  // keyboard_report[1] = reserved
  // keyboard_report[2-7] = key codes
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->keyboard_report_[0] = modifier;
  this->keyboard_report_[2] = keycode;

  this->send_report_();
  this->keypress_pending_ = false;
}

void HIDKeyboard::release_all() {
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->send_report_();
  this->keypress_pending_ = false;
}

void HIDKeyboard::send_keypress(uint8_t keycode, uint8_t modifier) {
  // Send the key press immediately
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->keyboard_report_[0] = modifier;
  this->keyboard_report_[2] = keycode;
  this->send_report_();

  // Schedule automatic release after KEY_RELEASE_DELAY_MS (non-blocking)
  this->keypress_pending_ = true;
  this->last_keypress_time_ = millis();
}

void HIDKeyboard::send_report_() {
  // Check if TinyUSB is ready
  if (!tud_ready()) {
    ESP_LOGW(TAG, "TinyUSB not ready, cannot send report");
    return;
  }

  // Send HID keyboard report
  // Note: REPORT_ID_KEYBOARD is defined in TinyUSB's hid.h as 1
  if (!tud_hid_keyboard_report(REPORT_ID_KEYBOARD, this->keyboard_report_[0], &this->keyboard_report_[2])) {
    ESP_LOGW(TAG, "Failed to send HID report");
  }
}

}  // namespace esphome::hid_keyboard

#endif  // USE_ESP32_VARIANT_ESP32P4 || USE_ESP32_VARIANT_ESP32S2 || USE_ESP32_VARIANT_ESP32S3
