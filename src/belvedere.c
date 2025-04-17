#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <uv.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>

#include "../include/config.h"
#include "../include/debug.h"
#include "../include/hid_manager.h"

config_t config;
extern bool debug_enabled;
static char config_path[512];
static time_t last_config_mtime = 0;
static uv_timer_t poll_timer;
static uv_fs_poll_t config_watcher;
static uv_signal_t sighup_handler;

// Callback for key events
void handle_key_event(uint16_t vendor_id, uint16_t product_id, uint16_t keycode, void* user_data) {
    (void)user_data;  // Silence unused parameter warning
    debug("Key event: vendor_id=0x%04x, product_id=0x%04x, keycode=0x%x\n",
          vendor_id, product_id, keycode);

    // Get the mapped command for the key event
    const char *command = get_command_for_key(&config, vendor_id, product_id, keycode);
    if (command) {
        debug("Executing command: %s\n", command);
        system(command);
    } else {
        debug("No command mapped for keycode=%d\n", keycode);
    }
}

// Function to reload configuration
bool reload_configuration() {
    debug("Reloading configuration...\n");

    // Load new configuration
    if (!load_config(config_path, &config)) {
        debugf(stderr, "Failed to reload config.\n");
        return false;
    }

    if (config.monitored_keycodes_count < 1) {
        debugf(stderr, "monitored_keycodes are not defined in the configuration file.\n");
        return false;
    }

    if (config.device_count < 1) {
        debugf(stderr, "no devices are defined in the configuration file.\n");
        return false;
    }

    debug("Configuration reloaded successfully.\n");

    // Reload HID devices
    if (!hid_manager_reload()) {
        debugf(stderr, "Failed to reload HID devices.\n");
        return false;
    }

    debug("HID devices reloaded successfully.\n");
    return true;
}

// Callback for configuration file changes
void on_config_change(uv_fs_poll_t* handle, int status, const uv_stat_t* prev, const uv_stat_t* curr) {
    (void)handle;  // Silence unused parameter warning
    (void)prev;    // Silence unused parameter warning
    if (status < 0) {
        debugf(stderr, "Error watching config file: %s\n", uv_strerror(status));
        return;
    }

    time_t curr_mtime = curr->st_mtim.tv_sec;
    if (curr_mtime > last_config_mtime) {
        debug("Configuration file has changed, reloading...\n");
        last_config_mtime = curr_mtime;
        reload_configuration();
    }
}

// Callback for SIGHUP signal
void on_sighup(uv_signal_t* handle, int signum) {
    (void)handle;   // Silence unused parameter warning
    (void)signum;   // Silence unused parameter warning
    debug("Received SIGHUP signal, reloading configuration...\n");
    reload_configuration();
}

// Timer callback for polling devices
void poll_devices(uv_timer_t* handle) {
    (void)handle;  // Silence unused parameter warning
    hid_manager_poll();
}

// Cleanup function
void cleanup(uv_handle_t* handle) {
    free(handle);
}

int main(int argc, char *argv[]) {
    // Check for the -v flag to enable debug logging
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            debug_enabled = true;
            debug("Debug logging enabled.\n");
        } else if (strcmp(argv[i], "--reload") == 0) {
            // If --reload flag is provided, just reload and exit
            const char *home = getenv("HOME");
            if (!home) {
                debugf(stderr, "Failed to retrieve HOME environment variable.\n");
                return 1;
            }

            snprintf(config_path, sizeof(config_path), "%s/.config/belvedere/config", home);
            if (reload_configuration()) {
                printf("Configuration reloaded successfully.\n");
                return 0;
            } else {
                printf("Failed to reload configuration.\n");
                return 1;
            }
        }
    }

    const char *home = getenv("HOME");
    if (!home) {
        debugf(stderr, "Failed to retrieve HOME environment variable.\n");
        return 1;
    }

    snprintf(config_path, sizeof(config_path), "%s/.config/belvedere/config", home);

    if (!load_config(config_path, &config)) {
        debugf(stderr, "Failed to load config.\n");
        return 1;
    }

    if (config.monitored_keycodes_count < 1) {
        debugf(stderr, "monitored_keycodes are not defined in the configuration file.\n");
        return false;
    }

    if (config.device_count < 1) {
        debugf(stderr, "no devices are defined in the configuration file.\n");
        return false;
    }

    debug("Configuration loaded successfully.\n");

    // Initialize libuv loop
    uv_loop_t* loop = uv_default_loop();

    // Initialize HID manager
    if (!hid_manager_init()) {
        debugf(stderr, "Failed to initialize HID manager.\n");
        return 1;
    }

    // Set up key event callback
    hid_manager_set_key_callback(handle_key_event, NULL);

    // Set up device polling timer
    uv_timer_init(loop, &poll_timer);
    uv_timer_start(&poll_timer, poll_devices, 0, 10);  // Poll every 10ms

    // Set up configuration file watcher
    uv_fs_poll_init(loop, &config_watcher);
    uv_fs_poll_start(&config_watcher, on_config_change, config_path, 1000);  // Check every second

    // Set up SIGHUP handler
    uv_signal_init(loop, &sighup_handler);
    uv_signal_start(&sighup_handler, on_sighup, SIGHUP);

    debug("Listening for input events...\n");

    // Run the event loop
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup
    uv_fs_poll_stop(&config_watcher);
    uv_signal_stop(&sighup_handler);
    uv_timer_stop(&poll_timer);
    uv_close((uv_handle_t*)&config_watcher, cleanup);
    uv_close((uv_handle_t*)&sighup_handler, cleanup);
    uv_close((uv_handle_t*)&poll_timer, cleanup);
    uv_loop_close(loop);

    hid_manager_cleanup();
    return 0;
}

#include <stdlib.h>
#include <string.h>