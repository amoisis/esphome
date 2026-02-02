#if defined(USE_ESP32_VARIANT_ESP32P4) || defined(USE_ESP32_VARIANT_ESP32S2) || defined(USE_ESP32_VARIANT_ESP32S3)

#include "hid_keyboard.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cstring>

// Include TinyUSB HID APIs from esp_tinyusb
#include "tusb.h"
#include "class/hid/hid.h"

namespace esphome::hid_keyboard {

static const char *const TAG = "hid_keyboard";

// HID Report ID for keyboard
#ifndef REPORT_ID_KEYBOARD
#define REPORT_ID_KEYBOARD 1
#endif

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
  // Check if TinyUSB is ready and device is connected
  if (!tud_hid_ready()) {
    ESP_LOGD(TAG, "HID not ready, cannot send report");
    return;
  }

  // Send HID keyboard report to host
  if (!tud_hid_keyboard_report(REPORT_ID_KEYBOARD, this->keyboard_report_[0], &this->keyboard_report_[2])) {
    ESP_LOGD(TAG, "Failed to send HID report");
  }
}

}  // namespace esphome::hid_keyboard

// TinyUSB HID Callback Functions
// These are required by TinyUSB's HID device class implementation

// Invoked when received GET HID REPORT DESCRIPTOR request
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) {
  // Return NULL to use the built-in keyboard descriptor from TinyUSB
  return NULL;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  // Not used for keyboard
  return 0;
}

// Invoked when received SET_REPORT control request
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t buflen) {
  // Not used for keyboard (LED output reports would be handled here)
}

#endif  // USE_ESP32_VARIANT_ESP32P4 || USE_ESP32_VARIANT_ESP32S2 || USE_ESP32_VARIANT_ESP32S3
