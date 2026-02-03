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
// Set to 0 because TUD_HID_REPORT_DESC_KEYBOARD() without arguments uses no report ID
#ifndef REPORT_ID_KEYBOARD
#define REPORT_ID_KEYBOARD 0
#endif

void HIDKeyboard::setup() {
  ESP_LOGI(TAG, "Setting up HID Keyboard");
  this->init_hid_();
}

void HIDKeyboard::loop() {
  // Update USB connection status sensor if configured
  if (this->status_sensor_ != nullptr) {
    bool is_ready = tud_hid_ready();
    if (this->status_sensor_->state != is_ready) {
      this->status_sensor_->publish_state(is_ready);
    }
  }
}

void HIDKeyboard::dump_config() { ESP_LOGCONFIG(TAG, "HID Keyboard:"); }

void HIDKeyboard::init_hid_() {
  // Initialize keyboard report buffer
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
}

void HIDKeyboard::send_key(uint8_t keycode, uint8_t modifier) {
  // keyboard_report[0] = modifier keys
  // keyboard_report[1] = reserved
  // keyboard_report[2-7] = key codes
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->keyboard_report_[0] = modifier;
  this->keyboard_report_[2] = keycode;

  this->send_report_();
}

void HIDKeyboard::release_all() {
  memset(this->keyboard_report_, 0, sizeof(this->keyboard_report_));
  this->send_report_();
}

void HIDKeyboard::send_keypress(uint8_t keycode, uint8_t modifier) {
  this->send_key(keycode, modifier);
  this->set_timeout("release_key", KEY_RELEASE_DELAY_MS, [this]() { this->release_all(); });
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
// These must be in extern "C" because TinyUSB is written in C
// and needs to find these symbols during linking
extern "C" {

// Standard HID Keyboard Report Descriptor
// This tells the PC what buttons/keys this device has
static const uint8_t desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

// Invoked when received GET HID REPORT DESCRIPTOR request
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) { return desc_hid_report; }

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

}  // extern "C"

#endif  // USE_ESP32_VARIANT_ESP32P4 || USE_ESP32_VARIANT_ESP32S2 || USE_ESP32_VARIANT_ESP32S3
