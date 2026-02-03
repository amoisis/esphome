# HID Keyboard Component - Setup Guide

## Quick Start

The HID Keyboard component allows your ESP32-S2/S3/P4 to act as a USB keyboard device.

### Minimum Configuration

```yaml
esphome:
  name: my-hid-keyboard

esp32:
  board: esp32-s3-devkitc-1
  variant: ESP32S3
  framework:
    type: esp-idf  # REQUIRED: Must use ESP-IDF, not Arduino

# CRITICAL: Use UART0 for logging to avoid USB-JTAG conflict
logger:
  hardware_uart: UART0

# REQUIRED: TinyUSB component must be configured first
tinyusb:
  usb_product_id: 0x4001
  usb_vendor_id: 0x303A
  usb_manufacturer_str: "ESPHome"
  usb_product_str: "HID Keyboard"

# HID Keyboard component
hid_keyboard:
  id: my_keyboard
```

## Hardware Requirements

### Supported Boards
- ✅ ESP32-S3 (all variants with USB-OTG)
- ✅ ESP32-S2 (all variants with USB-OTG)
- ✅ ESP32-P4 (with USB-OTG support)
- ❌ ESP32 (original) - No USB-OTG support
- ❌ ESP32-C3/C6/H2 - No USB-OTG support

### USB Connection
You **must** use the USB-OTG port, not the USB-JTAG port:
- **ESP32-S3 DevKit**: Usually the port labeled "USB" (not "UART")
- **After flashing via UART**, disconnect and reconnect to the USB-OTG port
- Some boards have only one port that switches between JTAG and OTG modes

## Common Issues & Solutions

### ❌ "tinyusb is marked FAILED"

**Cause**: USB-JTAG console conflicts with USB-OTG device mode.

**Solution**: Add this to your logger configuration:
```yaml
logger:
  hardware_uart: UART0  # Forces logs to UART instead of USB-JTAG
```

### ❌ "HID not ready, cannot send report"

**Cause**: TinyUSB component not initialized or USB cable not connected.

**Solutions**:
1. Ensure `tinyusb:` component is configured in your YAML
2. Connect USB-OTG port (not JTAG port)
3. Check Device Manager (Windows) or `lsusb` (Linux) for USB HID device
4. Perform a hard power cycle (unplug, wait 5 seconds, replug)

### ❌ Keyboard not detected by computer

**Cause**: Wrong USB port or configuration issue.

**Solutions**:
1. **Verify correct USB port**: Use USB-OTG, not UART/JTAG
2. **Check logs**: Look for TinyUSB initialization messages
3. **Try different cable**: Some cables are charge-only
4. **Test on different OS**: Verify with Windows Device Manager or Linux dmesg

### ❌ Component not found: tinyusb

**Cause**: Using ESPHome version without tinyusb component.

**Solution**: Update to ESPHome 2025.7.0 or later, or check that your installation includes the tinyusb component.

## USB Status Monitoring

Optionally monitor USB connection status:

```yaml
hid_keyboard:
  id: my_keyboard
  status_sensor:
    name: "Keyboard USB Status"
```

This binary sensor will be:
- `ON` when USB is connected and ready
- `OFF` when USB is disconnected

## Flashing & Testing Workflow

1. **Initial Flash via UART**:
   ```bash
   esphome run my-keyboard.yaml
   ```

2. **Disconnect UART**, connect to **USB-OTG port**

3. **Hard power cycle**: Unplug, wait 5 seconds, replug

4. **Verify in OS**:
   - **Windows**: Device Manager → Human Interface Devices → "HID Keyboard Device"
   - **Linux**: `lsusb | grep 303A` should show your device
   - **macOS**: System Information → USB → Look for ESPHome device

5. **Test keypresses** from Home Assistant or web interface

## Example Configurations

See the example files in this directory:
- `minimal-test.yaml` - Quick testing configuration
- `test-example.yaml` - Comprehensive feature demonstration
- `macro-pad-example.yaml` - Physical button macro keyboard
- `example.yaml` - Basic template with common shortcuts

## Technical Details

### HID Report Structure
- 8 bytes total
- Byte 0: Modifier keys (Ctrl, Shift, Alt, GUI)
- Byte 1: Reserved (always 0)
- Bytes 2-7: Up to 6 simultaneous key presses (6-key rollover)

### TinyUSB Integration
This component uses the ESP-IDF `espressif/esp_tinyusb` component (version 1.7.6~1) for USB device functionality. The `tinyusb` component handles:
- USB device enumeration
- String descriptors
- Device configuration
- USB power management

### Non-blocking Operation
The `send_keypress` action automatically releases keys after 50ms using ESPHome's scheduler, ensuring no blocking delays in your automations.

## Further Resources

- [TinyUSB Documentation](https://docs.tinyusb.org/)
- [USB HID Usage Tables](https://usb.org/sites/default/files/hut1_5.pdf)
- [ESPHome Components](https://esphome.io/components/)
