#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <errno.h>

#include "../include/config.h"
#include "../include/debug.h"
#include "../include/device_utils.h"
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDManager.h>

static IOHIDManagerRef hidManager = NULL;

void HIDInputCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value) {
    IOHIDDeviceRef device = IOHIDElementGetDevice(IOHIDValueGetElement(value));
    uint32_t usagePage = IOHIDElementGetUsagePage(IOHIDValueGetElement(value));
    uint32_t usage = IOHIDElementGetUsage(IOHIDValueGetElement(value));

    if (usagePage == kHIDPage_KeyboardOrKeypad) {
        debug("Received key event: usage=0x%x (%d)\n", usage, usage);
        uint16_t *vendor_id = (uint16_t *)context;
        uint16_t *product_id = vendor_id + 1;

        CFNumberRef vendor_id_ref = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
        CFNumberRef product_id_ref = (CFNumberRef)IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));

        if (vendor_id_ref) {
            CFNumberGetValue(vendor_id_ref, kCFNumberSInt32Type, vendor_id);
        } else {
            debugf(stderr, "Could not get vendor ID IOHIDDevice.\n");
            return;
        }

        if (product_id_ref) {
            CFNumberGetValue(product_id_ref, kCFNumberSInt32Type, product_id);
        } else {
            debugf(stderr, "Could not get product ID IOHIDDevice.\n");
            return;
        }

        extern config_t config;
        bool keycode_monitored = false;

        // Convert usage to QMK-style keycode (SAFE_RANGE + usage)
        // QMK's SAFE_RANGE is typically 0x7700
        uint16_t qmk_keycode = 0x7700 + usage;

        // Check if this is a QMK custom keycode (in the SAFE_RANGE range)
        bool is_qmk_custom_keycode = (qmk_keycode >= 0x7700 && qmk_keycode < 0x7800);

        if (is_qmk_custom_keycode) {
            // Automatically monitor all QMK custom keycodes
            keycode_monitored = true;
            debug("Automatically monitoring QMK custom keycode: 0x%04x\n", qmk_keycode);
        } else {
            // For non-QMK keycodes, check if they're in the monitored list
            for (size_t i = 0; i < config.monitored_keycodes_count; i++) {
                debug("Comparing usage=%d with monitored_keycode=%d\n", usage, config.monitored_keycodes[i]);
                if (usage == config.monitored_keycodes[i]) {
                    keycode_monitored = true;
                    break;
                }
            }
        }

        if (!keycode_monitored) {
            debug("Keycode %d (QMK: 0x%04x) not in monitored list, ignoring event.\n", usage, qmk_keycode);
            return;
        } else {
            debug("Key event: vendor_id=0x%04x, product_id=0x%04x, usage=0x%x (keycode=%d, QMK=0x%04x)\n",
                    *vendor_id, *product_id, usage, usage, qmk_keycode);
        }

        // Get the mapped command for the key event
        const char *command = NULL;

        // For QMK custom keycodes, try the QMK keycode first
        if (is_qmk_custom_keycode) {
            command = get_command_for_key(&config, *vendor_id, *product_id, (uint16_t)qmk_keycode);
        }

        // If no command found or not a QMK keycode, try the raw usage
        if (!command) {
            command = get_command_for_key(&config, *vendor_id, *product_id, (uint8_t)usage);
        }

        if (command) {
            debug("Executing command: %s\n", command);
            system(command);
        } else {
            debug("No command mapped for keycode=%d (QMK=0x%04x)\n", usage, qmk_keycode);
        }
    }
}

kern_return_t initialize_hid_manager(uint16_t *vendor_id, uint16_t *product_id) {
    hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!hidManager) {
        debugf(stderr, "Failed to create IOHIDManager.\n");
        return kIOReturnError;
    }

    // Create a matching dictionary for keyboards
    CFMutableDictionaryRef matching = CFDictionaryCreateMutable(kCFAllocatorDefault,
        0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    int usagePage = kHIDPage_GenericDesktop;
    int usage = kHIDUsage_GD_Keyboard;
    CFDictionarySetValue(matching, CFSTR(kIOHIDDeviceUsagePageKey), CFNumberCreate(NULL, kCFNumberIntType, &usagePage));
    CFDictionarySetValue(matching, CFSTR(kIOHIDDeviceUsageKey), CFNumberCreate(NULL, kCFNumberIntType, &usage));

    IOHIDManagerSetDeviceMatching(hidManager, matching);
    CFRelease(matching);

    // Register the input callback
    IOHIDManagerRegisterInputValueCallback(hidManager, HIDInputCallback, vendor_id);

    // Open the HID manager
    IOReturn result = IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess) {
        debugf(stderr, "Failed to open IOHIDManager: 0x%x\n", result);
        CFRelease(hidManager);
        return result;
    }

    // Schedule the HID manager on the run loop
    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    debug("HID Manager initialized and scheduled on run loop.\n");

    return kIOReturnSuccess;
}

void cleanup_hid_manager() {
    if (hidManager) {
        IOHIDManagerUnscheduleFromRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
        CFRelease(hidManager);
        hidManager = NULL;
        debug("HID Manager cleaned up.\n");
    }
}