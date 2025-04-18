#include "hid_manager.h"

#include <hidapi/hidapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "config.h"
#include "debug.h"

#define BUFFER_SIZE 64
#define MAX_ACTIVE_DEVICES 16

// Forward declarations
static void poll_devices(uv_timer_t* handle);

// Global configuration
extern config_t config;

// Global variables
static struct
{
    hid_device* devices[MAX_ACTIVE_DEVICES];
    int device_count;
    key_callback_t key_callback;
    void* user_data;
    uv_timer_t* poll_timer;
} hid_manager = {0};

// Platform-specific device matching
static bool match_device(struct hid_device_info* dev_info, device_config_t* config)
{
    return dev_info->vendor_id == config->vendor && dev_info->product_id == config->product;
}

bool hid_manager_init(void)
{
    // Initialize HIDAPI library
    if (hid_init() != 0)
    {
        debug("Failed to initialize HIDAPI");
        return false;
    }

    // Initialize polling timer
    hid_manager.poll_timer = malloc(sizeof(uv_timer_t));
    if (!hid_manager.poll_timer)
    {
        debug("Failed to allocate timer");
        hid_exit();
        return false;
    }

    uv_timer_init(uv_default_loop(), hid_manager.poll_timer);
    uv_timer_start(hid_manager.poll_timer, poll_devices, 0, 10);  // Poll every 10ms

    return true;
}

void hid_manager_cleanup(void)
{
    // Stop and free timer
    if (hid_manager.poll_timer)
    {
        uv_timer_stop(hid_manager.poll_timer);
        free(hid_manager.poll_timer);
        hid_manager.poll_timer = NULL;
    }

    // Close all devices
    for (int i = 0; i < hid_manager.device_count; i++)
    {
        if (hid_manager.devices[i])
        {
            hid_close(hid_manager.devices[i]);
            hid_manager.devices[i] = NULL;
        }
    }
    hid_manager.device_count = 0;

    // Cleanup HIDAPI
    hid_exit();
}

void hid_manager_set_key_callback(key_callback_t callback, void* user_data)
{
    hid_manager.key_callback = callback;
    hid_manager.user_data = user_data;
}

static void poll_devices(uv_timer_t* handle)
{
    (void)handle;  // Silence unused parameter warning
    unsigned char buf[BUFFER_SIZE];

    for (int i = 0; i < hid_manager.device_count; i++)
    {
        if (!hid_manager.devices[i])
            continue;

        int res = hid_read_timeout(hid_manager.devices[i], buf, sizeof(buf), 0);
        if (res > 0 && hid_manager.key_callback)
        {
            // Extract vendor and product ID for the device
            struct hid_device_info* info = hid_get_device_info(hid_manager.devices[i]);
            if (info)
            {
                // Assuming buf[0] contains the keycode - adjust based on your HID report format
                hid_manager.key_callback(info->vendor_id, info->product_id, buf[0],
                                         hid_manager.user_data);
            }
        }
    }
}

bool hid_manager_reload(void)
{
    // Close existing devices
    for (int i = 0; i < hid_manager.device_count; i++)
    {
        if (hid_manager.devices[i])
        {
            hid_close(hid_manager.devices[i]);
            hid_manager.devices[i] = NULL;
        }
    }
    hid_manager.device_count = 0;

    // Enumerate and open configured devices
    for (size_t i = 0; i < config.device_count && i < MAX_ACTIVE_DEVICES; i++)
    {
        struct hid_device_info* devs = hid_enumerate(0, 0);
        struct hid_device_info* cur_dev;

        for (cur_dev = devs; cur_dev; cur_dev = cur_dev->next)
        {
            if (match_device(cur_dev, &config.devices[i]))
            {
                hid_manager.devices[hid_manager.device_count] = hid_open_path(cur_dev->path);
                if (hid_manager.devices[hid_manager.device_count])
                {
                    hid_manager.device_count++;
                }
                break;
            }
        }

        hid_free_enumeration(devs);
    }

    return true;
}

void hid_manager_poll(void)
{
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
}