#if defined(USE_ESP32_VARIANT_ESP32P4) || defined(USE_ESP32_VARIANT_ESP32S2) || defined(USE_ESP32_VARIANT_ESP32S3)

#include "hid_keyboard.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cstring>

// Include TinyUSB HID APIs from esp_tinyusb
#include "tusb.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "tinyusb.h"

namespace esphome::hid_keyboard {

static const char *const TAG = "hid_keyboard";

// HID Configuration Descriptor
// Defines 1 configuration with 1 HID interface
// Length calculation: Config + (Number of HID interfaces * HID Descriptor Length)
// CFG_TUD_HID is defined by CONFIG_TINYUSB_HID_COUNT in sdkconfig
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

// Standard HID Keyboard Report Descriptor
static const uint8_t desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

// Used for configuration descriptor
// Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
// 0x81 is the standard instruction for EP1 IN
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x81, 16, 10),
};

static const char *const hid_string_descriptor[] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "ESPHome",             // 1: Manufacturer
    "HID Keyboard",        // 2: Product
    "123456",              // 3: Serials, should use chip ID
    "HID Interface",       // 4: HID
};

// HID Report ID for keyboard
// Set to 0 because TUD_HID_REPORT_DESC_KEYBOARD() without arguments uses no report ID
#ifndef REPORT_ID_KEYBOARD
#define REPORT_ID_KEYBOARD 0
#endif

void HIDKeyboard::setup() {
  ESP_LOGI(TAG, "=== HID Keyboard Setup Starting ===");

  this->init_hid_();

  // For single-USB-port boards (SuperMini), give USB peripheral time to initialize
  // The USB peripheral needs time to switch from JTAG mode (programming) to OTG mode (device)
  // sdkconfig options (CONFIG_TINYUSB_DEVICE_MODE, CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED)
  // should have configured the USB peripheral at startup
  ESP_LOGI(TAG, "Waiting 1 second for USB peripheral initialization...");
  delay(1000);

  // Check TinyUSB initialization state
  ESP_LOGI(TAG, "Checking TinyUSB status...");

  // These functions will tell us if TinyUSB driver is installed and working
  bool hid_ready = tud_hid_ready();
  bool usb_mounted = tud_mounted();
  bool usb_suspended = tud_suspended();

  ESP_LOGI(TAG, "TinyUSB Status:");
  ESP_LOGI(TAG, "  - HID ready: %d", hid_ready);
  ESP_LOGI(TAG, "  - USB mounted: %d", usb_mounted);
  ESP_LOGI(TAG, "  - USB suspended: %d", usb_suspended);

  this->hid_available_ = hid_ready && usb_mounted;

  if (!this->hid_available_) {
    ESP_LOGW(TAG, "HID not available yet - will retry in loop");
    if (!usb_mounted) {
      ESP_LOGW(TAG, "CRITICAL: USB not mounting to host!");
      ESP_LOGW(TAG, "Possible causes:");
      ESP_LOGW(TAG, "  1. TinyUSB driver installation failed (check ESP_ERROR logs above)");
      ESP_LOGW(TAG, "  2. USB cable not connected or faulty");
      ESP_LOGW(TAG, "  3. SuperMini hardware limitation - USB stuck in JTAG mode");
    }
    if (!hid_ready) {
      ESP_LOGW(TAG, "HID interface not ready - TinyUSB HID class not initialized");
    }
  } else {
    ESP_LOGI(TAG, "SUCCESS: HID device ready and USB enumerated on startup!");
  }

  ESP_LOGI(TAG, "=== HID Keyboard Setup Complete ===");
}

void HIDKeyboard::loop() {
  // Check if HID became available
  if (!this->hid_available_) {
    // Check every 1 second if USB enumeration completed
    static uint32_t last_check = 0;
    uint32_t now = millis();
    if (now - last_check > 1000) {
      last_check = now;
      bool hid_ready = tud_hid_ready();
      bool usb_mounted = tud_mounted();

      if (hid_ready && usb_mounted) {
        this->hid_available_ = true;
        ESP_LOGI(TAG, "HID device is now ready (USB enumerated successfully)");
      } else {
        // Periodic diagnostics
        ESP_LOGD(TAG, "Waiting for USB enumeration... HID ready=%d, USB mounted=%d", hid_ready, usb_mounted);
        if (!usb_mounted && !this->warned_not_mounted_) {
          this->warned_not_mounted_ = true;
          ESP_LOGW(TAG, "USB not mounting to host - SuperMini may be stuck in JTAG mode");
          ESP_LOGW(TAG, "Try: holding BOOT button to confirm JTAG works, then reboot without BOOT button");
        }
      }
    }
  }
}

void HIDKeyboard::dump_config() { ESP_LOGCONFIG(TAG, "HID Keyboard:"); }

void HIDKeyboard::init_hid_() {
  ESP_LOGI(TAG, "Initializing TinyUSB Driver...");

  tinyusb_config_t tusb_cfg = {0};
  tusb_cfg.device_descriptor = NULL;
  tusb_cfg.string_descriptor = hid_string_descriptor;
  tusb_cfg.string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
  tusb_cfg.external_phy = false;
#if (TUD_OPT_HIGH_SPEED)
  tusb_cfg.fs_configuration_descriptor = hid_configuration_descriptor;
  tusb_cfg.hs_configuration_descriptor = hid_configuration_descriptor;
  tusb_cfg.qualifier_descriptor = NULL;
#else
  tusb_cfg.configuration_descriptor = hid_configuration_descriptor;
#endif

  esp_err_t err = tinyusb_driver_install(&tusb_cfg);
  if (err != ESP_OK) {
    if (err == ESP_ERR_INVALID_STATE) {
      ESP_LOGW(TAG, "TinyUSB driver already installed");
    } else {
      ESP_LOGE(TAG, "Failed to install TinyUSB driver: %s", esp_err_to_name(err));
    }
  } else {
    ESP_LOGI(TAG, "TinyUSB driver installed successfully");
  }

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
  // Safety check: Verify TinyUSB is available before attempting to use HID
  // tud_hid_ready() will return false if TinyUSB initialization failed or USB is not connected
  if (!tud_hid_ready()) {
    ESP_LOGD(TAG, "HID not ready, cannot send report (TinyUSB may be failed or USB disconnected)");
    return;
  }

  // Send HID keyboard report to host
  // tud_hid_keyboard_report expects (report_id, modifier, keycode[6])
  if (!tud_hid_keyboard_report(REPORT_ID_KEYBOARD, this->keyboard_report_[0], &this->keyboard_report_[2])) {
    ESP_LOGD(TAG, "Failed to send HID report");
  }
}

}  // namespace esphome::hid_keyboard

// TinyUSB HID Callback Functions
// These must be in extern "C" because TinyUSB is written in C
// and needs to find these symbols during linking
extern "C" {

// Invoked when received GET HID REPORT DESCRIPTOR request
uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) { return esphome::hid_keyboard::desc_hid_report; }

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
