#pragma once

#if defined(USE_ESP32_VARIANT_ESP32P4) || defined(USE_ESP32_VARIANT_ESP32S2) || defined(USE_ESP32_VARIANT_ESP32S3)

#include "esphome/core/automation.h"
#include "hid_keyboard.h"

namespace esphome::hid_keyboard {

template<typename... Ts> class SendKeyAction : public Action<Ts...> {
 public:
  explicit SendKeyAction(HIDKeyboard *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(uint8_t, key)
  TEMPLATABLE_VALUE(uint8_t, modifier)

  void play(Ts... x) override {
    auto key = this->key_.value(x...);
    auto modifier = this->modifier_.value(x...);
    this->parent_->send_key(key, modifier);
  }

 protected:
  HIDKeyboard *parent_;
};

template<typename... Ts> class SendKeypressAction : public Action<Ts...> {
 public:
  explicit SendKeypressAction(HIDKeyboard *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(uint8_t, key)
  TEMPLATABLE_VALUE(uint8_t, modifier)

  void play(Ts... x) override {
    auto key = this->key_.value(x...);
    auto modifier = this->modifier_.value(x...);
    this->parent_->send_keypress(key, modifier);
  }

 protected:
  HIDKeyboard *parent_;
};

template<typename... Ts> class ReleaseAllAction : public Action<Ts...> {
 public:
  explicit ReleaseAllAction(HIDKeyboard *parent) : parent_(parent) {}

  void play(Ts... x) override { this->parent_->release_all(); }

 protected:
  HIDKeyboard *parent_;
};

}  // namespace esphome::hid_keyboard

#endif  // USE_ESP32_VARIANT_ESP32P4 || USE_ESP32_VARIANT_ESP32S2 || USE_ESP32_VARIANT_ESP32S3
