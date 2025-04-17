#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <hidapi/hidapi.h>
#include <uv.h>
#include "config.h"

// Type definitions
typedef void (*key_callback_t)(uint16_t vendor_id, uint16_t product_id, uint16_t keycode, void* user_data);

// Public functions
bool hid_manager_init(void);
void hid_manager_cleanup(void);
bool hid_manager_reload(void);
void hid_manager_set_key_callback(key_callback_t callback, void* user_data);
void hid_manager_poll(void);

#endif // HID_MANAGER_H