# TinyUSB Failure - Diagnostic Guide

## What We Know

From your logs:
```
[13:55:17.147][E][component:226]:   tinyusb is marked FAILED: unspecified
[13:55:17.190][C][hid_keyboard:027]: HID Keyboard:
```

✅ **Good news**: HID Keyboard component initialized successfully
❌ **Issue**: TinyUSB driver failed to install during `setup()`

## Most Likely Cause

The TinyUSB driver initialization is failing, likely because:

1. **USB cable not connected** - Most common. Ensure USB-OTG port is connected to your computer.
2. **USB peripheral not available** - Less likely on ESP32-S3
3. **Configuration issue** - Possible but your config looks correct

## How to Debug

### Step 1: Recompile with Better Error Messages

I've updated the TinyUSB component to log the actual ESP error code. Recompile your project:

```bash
# In Home Assistant ESPHome dashboard, force a rebuild
# Or use esphome CLI:
esphome run config/kvm.yaml
```

### Step 2: Check Logs for Error Code

After reboot, look for a message like:
```
[timestamp][E][tinyusb:xxx]: tinyusb_driver_install failed with error code: 0xXXXX
```

Common error codes:
- `0x101` - ESP_ERR_INVALID_ARG - Invalid configuration
- `0x102` - ESP_ERR_INVALID_STATE - Already initialized
- `0x103` - ESP_ERR_INVALID_SIZE - Size mismatch
- `0x104` - ESP_ERR_NOT_FOUND - Resource not found
- `0x105` - ESP_ERR_NOT_SUPPORTED - Feature not supported

### Step 3: Add Explicit TinyUSB Configuration

Your current config is correct, but let's make it explicit. Update your kvm.yaml:

```yaml
tinyusb:
  usb_vendor_id: 0x303A
  usb_product_id: 0x4001
  usb_manufacturer_str: "ESPHome"
  usb_product_str: "HID Keyboard"
```

### Step 4: Ensure USB Cable is Connected

- **Connect the USB-OTG port** (the USB port that's NOT used for programming) to your computer
- **NOT** the USB-JTAG port used for flashing - that's for programming only
- On ESP32-S3-DevKit-C-1, the USB-OTG port is the "USB" port (bottom connector typically)

### Step 5: Check GPIO Conflicts

Your sdkconfig already correctly disables USB-JTAG:
```yaml
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG: "n"  # ✅ Correct
CONFIG_ESP_CONSOLE_UART_DEFAULT: "y"     # ✅ Correct
CONFIG_ESP_CONSOLE_UART_NUM: "0"         # ✅ Correct
```

This frees up GPIO19/20 for USB-OTG. Good!

## What if TinyUSB Still Fails?

Even if TinyUSB component is marked FAILED, your **HID Keyboard component can still work** if:
- The TinyUSB driver partially initialized
- USB enumeration happened before the failure
- Your device is already recognized by the host

Try pressing one of your test buttons and check if:
1. The host computer recognizes it as a keyboard
2. Keypresses are registered

## Workaround: Continue Testing

The component loading succeeded. Even if TinyUSB component is marked failed, you can:

1. **Test the buttons** - They may still work
2. **Check device enumeration** - Run `lsusb` on Linux/Mac or Device Manager on Windows
3. **Look for HID device** - It should appear as "ESPHome HID Keyboard" if USB enumeration succeeded

## Next Steps

1. ✅ Recompile (the improved error logging is already in place)
2. Connect USB-OTG cable if not already connected
3. Check the new error message in logs
4. Report the error code you see, and we can investigate further

## Key Point

⚠️ Even if the TinyUSB component shows FAILED in startup logs, the HID keyboard may still work if the USB enumeration completed before the failure occurred. Try testing a button press and check your computer's USB device list.
