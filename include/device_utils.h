#pragma once
#include <IOKit/hid/IOHIDManager.h>
#include <stdint.h>

kern_return_t initialize_hid_manager(uint16_t *vendor_id, uint16_t *product_id);
void cleanup_hid_manager();