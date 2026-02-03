from esphome import automation
import esphome.codegen as cg
from esphome.components import esp32
from esphome.components.esp32 import (
    VARIANT_ESP32P4,
    VARIANT_ESP32S2,
    VARIANT_ESP32S3,
    add_idf_component,
    add_idf_sdkconfig_option,
)
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@amoisis"]
DEPENDENCIES = ["tinyusb"]

CONF_KEY = "key"
CONF_MODIFIER = "modifier"

hid_keyboard_ns = cg.esphome_ns.namespace("hid_keyboard")
HIDKeyboard = hid_keyboard_ns.class_("HIDKeyboard", cg.Component)

SendKeyAction = hid_keyboard_ns.class_("SendKeyAction", automation.Action)
SendKeypressAction = hid_keyboard_ns.class_("SendKeypressAction", automation.Action)
ReleaseAllAction = hid_keyboard_ns.class_("ReleaseAllAction", automation.Action)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HIDKeyboard),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    esp32.only_on_variant(
        supported=[VARIANT_ESP32P4, VARIANT_ESP32S2, VARIANT_ESP32S3],
    ),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Ensure TinyUSB library is available and linked
    add_idf_component(name="espressif/esp_tinyusb", ref="1.7.6~1")

    # Enable TinyUSB HID device support in sdkconfig
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_ENABLED", True)
    # Set the number of HID interfaces (must be >= 1 for HID to be compiled)
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_COUNT", 1)
    # Ensure HID is available as a device interface
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_INTERNAL_FIFO", True)

    # CRITICAL: Force USB device mode activation for single-USB boards (SuperMini)
    # These options ensure the USB peripheral switches from JTAG to OTG device mode
    add_idf_sdkconfig_option("CONFIG_TINYUSB_DEVICE_MODE", True)
    add_idf_sdkconfig_option("CONFIG_USB_OTG_SUPPORTED_SPEED_HS", True)

    # For SuperMini boards: Disable USB JTAG to prevent mode conflicts
    # This forces the USB peripheral to stay in device mode, not JTAG mode
    add_idf_sdkconfig_option("CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED", False)
    add_idf_sdkconfig_option("CONFIG_USB_SERIAL_JTAG_ENABLED", False)

    # Additional USB OTG options to ensure proper device enumeration
    add_idf_sdkconfig_option(
        "CONFIG_TINYUSB_HID_PROTOCOL", "1"
    )  # HID Keyboard protocol
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_SUBCLASS", "1")  # Boot subclass
    add_idf_sdkconfig_option("CONFIG_TINYUSB_ENDPOINT0_SIZE", "64")
    add_idf_sdkconfig_option("CONFIG_TINYUSB_MAX_CONFIG_POWER", "500")


@automation.register_action(
    "hid_keyboard.send_key",
    SendKeyAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(HIDKeyboard),
            cv.Required(CONF_KEY): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_MODIFIER, default=0): cv.templatable(cv.uint8_t),
        }
    ),
)
async def send_key_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_KEY], args, cg.uint8)
    cg.add(var.set_key(template_))  # type: ignore
    template_ = await cg.templatable(config[CONF_MODIFIER], args, cg.uint8)
    cg.add(var.set_modifier(template_))  # type: ignore
    return var


@automation.register_action(
    "hid_keyboard.send_keypress",
    SendKeypressAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(HIDKeyboard),
            cv.Required(CONF_KEY): cv.templatable(cv.uint8_t),
            cv.Optional(CONF_MODIFIER, default=0): cv.templatable(cv.uint8_t),
        }
    ),
)
async def send_keypress_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_KEY], args, cg.uint8)
    cg.add(var.set_key(template_))  # type: ignore
    template_ = await cg.templatable(config[CONF_MODIFIER], args, cg.uint8)
    cg.add(var.set_modifier(template_))  # type: ignore
    return var


@automation.register_action(
    "hid_keyboard.release_all",
    ReleaseAllAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(HIDKeyboard),
        }
    ),
)
async def release_all_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)
