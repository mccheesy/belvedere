#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <hidapi/hidapi.h>
#include <uv.h>
#include "../include/hid_manager.h"
#include "../include/config.h"

// Simple test framework
#define TEST(name) void test_##name(void)
#define ASSERT(condition) assert(condition)
#define TEST_RUN(name) test_##name(); printf("✓ %s passed\n", #name)

// Global configuration
config_t config = {0};

// Mock HID device for testing
typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    unsigned char buffer[64];
    int buffer_size;
} mock_hid_device_t;

// Global mock device for testing
mock_hid_device_t mock_device = {
    .vendor_id = 0x5043,
    .product_id = 0x54a3,
    .buffer = {0, 0},
    .buffer_size = 2
};

// Mock HIDAPI functions
// In a real test, we would use a mocking framework like CMock
// For simplicity, we're just providing these implementations

// Mock hid_init
int mock_hid_init(void) {
    return 0;  // Success
}

// Mock hid_exit
void mock_hid_exit(void) {
    // Nothing to do
}

// Mock hid_enumerate
struct hid_device_info* mock_hid_enumerate(unsigned short vendor_id, unsigned short product_id) {
    (void)vendor_id;  // Silence unused parameter warning
    (void)product_id;  // Silence unused parameter warning

    // Create a mock device info
    struct hid_device_info* info = calloc(1, sizeof(struct hid_device_info));
    if (!info) return NULL;

    info->vendor_id = mock_device.vendor_id;
    info->product_id = mock_device.product_id;
    info->path = strdup("/dev/hidraw0");
    info->next = NULL;
    info->bus_type = HID_API_BUS_USB;

    return info;
}

// Mock hid_free_enumeration
void mock_hid_free_enumeration(struct hid_device_info* devs) {
    if (devs) {
        free(devs->path);
        free(devs);
    }
}

// Mock hid_open_path
hid_device* mock_hid_open_path(const char* path) {
    (void)path;  // Silence unused parameter warning
    // Return a non-NULL pointer to indicate success
    return (hid_device*)1;
}

// Mock hid_close
void mock_hid_close(hid_device* device) {
    (void)device;  // Silence unused parameter warning
    // Nothing to do
}

// Mock hid_read_timeout
int mock_hid_read_timeout(hid_device* device, unsigned char* data, size_t length, int milliseconds) {
    (void)device;  // Silence unused parameter warning
    (void)length;  // Silence unused parameter warning
    (void)milliseconds;  // Silence unused parameter warning

    // Copy our mock buffer to the provided buffer
    memcpy(data, mock_device.buffer, mock_device.buffer_size);
    return mock_device.buffer_size;
}

// Test HID manager initialization
TEST(hid_manager_init) {
    // Set up mock functions
    // In a real test, we would use a mocking framework
    // For simplicity, we're just declaring these as external

    // Initialize HID manager
    ASSERT(hid_manager_init() == true);

    // Clean up
    hid_manager_cleanup();
}

// Test HID device reloading
TEST(hid_manager_reload) {
    // Set up configuration
    config.device_count = 1;
    config.devices[0].vendor = 0x5043;
    config.devices[0].product = 0x54a3;

    // Initialize HID manager
    ASSERT(hid_manager_init() == true);

    // Reload devices
    ASSERT(hid_manager_reload() == true);

    // Clean up
    hid_manager_cleanup();
}

// Callback function for key events
static void test_callback(uint16_t vendor_id, uint16_t product_id, uint16_t keycode, void* user_data) {
    (void)vendor_id;  // Silence unused parameter warning
    (void)product_id;  // Silence unused parameter warning
    (void)keycode;  // Silence unused parameter warning
    int* callback_called = (int*)user_data;
    *callback_called = 1;
}

// Test key event callback
TEST(key_event_callback) {
    int callback_called = 0;

    // Set up mock device
    mock_device.buffer[0] = 0;  // Report ID
    mock_device.buffer[1] = 111;  // Keycode
    mock_device.buffer_size = 2;

    // Initialize HID manager
    ASSERT(hid_manager_init() == true);

    // Set callback
    hid_manager_set_key_callback(test_callback, &callback_called);

    // Simulate device polling
    hid_manager_poll();

    // Verify callback was called
    ASSERT(callback_called == 1);

    // Clean up
    hid_manager_cleanup();
}

int main() {
    printf("Running HID manager tests...\n");
    TEST_RUN(hid_manager_init);
    TEST_RUN(hid_manager_reload);
    TEST_RUN(key_event_callback);
    printf("All HID manager tests passed!\n");
    return 0;
}