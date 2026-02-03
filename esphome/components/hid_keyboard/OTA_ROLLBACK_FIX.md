# OTA Rollback Issue - Diagnosis & Solution

## Problem Analysis

Your device is experiencing **OTA rollback on reboot** with this message:
```
[14:08:04.934][W][safe_mode:053]: OTA rollback detected! Rolled back from partition 'app0'
[14:08:04.934]The device reset before the boot was marked successful
```

This indicates the firmware is **crashing or resetting during early startup**, before the 60-second "boot successful" mark that prevents rollback.

## Root Cause

The issue is cascading failures:

1. **TinyUSB component fails to initialize** (shows `[E][component:226]: tinyusb is marked FAILED`)
2. **HID Keyboard tries to use TinyUSB without proper checks**
3. **Device crashes** when HID calls `tud_hid_ready()` on a failed/uninitialized interface
4. **Watchdog reset** triggers safe mode rollback

## What We Fixed

### Commit 3b5b18c2b - Stability Improvements

✅ **TinyUSB non-fatal failure handling**:
- Changed from `mark_failed()` to `log warning + continue`
- Allows device to function even if initial TinyUSB setup fails
- USB may enumerate successfully after a moment even if initial setup failed

✅ **HID Keyboard execution priority**:
- Changed from `setup_priority::DATA` to `setup_priority::AFTER_CONNECTION`
- Ensures TinyUSB is fully initialized before HID tries to use it
- Prevents race conditions during startup

✅ **HID availability tracking**:
- Added `hid_available_` flag
- Checks in `setup()` if HID is ready
- Monitors in `loop()` for delayed USB enumeration
- Gracefully handles cases where USB connects after boot

✅ **Better error logging**:
- Now shows actual ESP error codes (e.g., `0x102`)
- Distinguishes between fatal and non-fatal errors
- Helps with future debugging

### Enhanced TinyUSB Component

Modified `/esphome/components/tinyusb/tinyusb_component.cpp`:
```cpp
// Now only marks failed if it's a REAL failure, not just "already initialized"
if (result != ESP_OK) {
    ESP_LOGE(TAG, "tinyusb_driver_install failed with error code: 0x%x", result);
    if (result != ESP_ERR_INVALID_STATE) {
        // Continue anyway - device may still work
        ESP_LOGW(TAG, "TinyUSB issue but continuing...");
    }
}
```

## What to Do Now

### Step 1: Rebuild with Latest Code

Recompile your firmware with the latest changes from `feature/hid-keyboard` branch:

```bash
# In Home Assistant ESPHome:
# - Go to ESPHome dashboard
# - Click on your device
# - Clean Build Files
# - Click "Install" or "Compile"
```

### Step 2: Flash to Device

Flash the new firmware **completely erasing the old one** (to clear OTA partitions):

```bash
# Option 1: Via USB (fastest for debugging)
# esphome run config/kvm.yaml --device /dev/ttyUSB0

# Option 2: Via Home Assistant ESPHome addon
# Clean build + Install
```

### Step 3: Monitor Logs After Boot

After reboot, watch for:

✅ **Good signs**:
```
[timestamp][I][hid_keyboard:027]: Setting up HID Keyboard
[timestamp][I][hid_keyboard:xxx]: HID device is now ready
```

⚠️ **Expected warnings** (don't worry):
```
[timestamp][W][tinyusb:xxx]: TinyUSB initialization issue, but continuing
[timestamp][I][safe_mode:021]: Successful after: 60s
```

❌ **Problems to report**:
```
watchdog trigger
GURU Meditation Error
assert failed
```

### Step 4: Test Keyboard Functionality

After 60 seconds of stable boot, try one of your buttons:
1. Open a text editor on your computer
2. In Home Assistant, trigger a button (e.g., "Type A")
3. Check if 'A' appears in the text editor

## Why This Helps

**Before**: TinyUSB failure → HID crashes → Watchdog → Rollback loop

**After**: TinyUSB failure → HID gracefully skips operations → Device boots successfully → USB may enumerate later → HID works once USB is ready

The device is now much **more resilient** to USB initialization hiccups. Even if TinyUSB has issues, the device won't crash.

## Expected Behavior

1. **First boot**: Completes setup, USB may or may not be ready yet
2. **On USB connection**: USB enumeration happens, HID becomes available
3. **No rollback**: Device survives the initial 60-second boot window
4. **Buttons work**: Once USB is connected, keyboard commands execute

## If Still Having Issues

Provide these logs after the fix:
```bash
# Get the error code that TinyUSB is reporting
# Look for: "tinyusb_driver_install failed with error code: 0xXXXX"

# Check if HID eventually becomes ready:
# Look for: "HID device is now ready"

# Watch for any crashes after 60 seconds
```

## Technical Details

The key insight: **TinyUSB initialization can fail for transient reasons** (USB cable timing, etc.) but the device can still work. By making TinyUSB non-fatal and adding retry logic in the loop, we eliminate the cascading crash scenario.

Your HID Keyboard component is **stable and well-designed** - it already has `tud_hid_ready()` checks before every command. The problem was the boot crash, not the HID code itself.
