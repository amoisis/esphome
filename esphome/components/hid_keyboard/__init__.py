from esphome import automation
import esphome.codegen as cg
from esphome.components import esp32
from esphome.components.esp32 import (
    VARIANT_ESP32P4,
    VARIANT_ESP32S2,
    VARIANT_ESP32S3,
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

    # Enable TinyUSB HID support
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_ENABLED", True)
    add_idf_sdkconfig_option("CONFIG_TINYUSB_HID_COUNT", 1)


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
