#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // for size_t

#define MAX_BINDINGS 5
#define MAX_DEVICES 5
#define MAX_MONITORED_KEYCODES 5
#define MAX_PATH 256

#define DEFAULT_SETLEDS_PATH "/usr/local/bin/setleds"

typedef struct {
    uint16_t keycode;
    char led[16];      // "caps", "num", "scroll"
    char mode;         // '^', '+', or '-'
    bool has_mode_override;
} key_binding_t;

typedef struct {
    uint16_t vendor;
    uint16_t product;
    char target[128];    // wildcard match name for target device
    char default_mode;
    key_binding_t bindings[10];
    size_t binding_count;
} device_config_t;

typedef struct {
    char setleds_path[MAX_PATH];
    device_config_t devices[10];
    size_t device_count;
    uint32_t monitored_keycodes[MAX_MONITORED_KEYCODES];
    size_t monitored_keycodes_count;
} config_t;

extern config_t config;

/**
 * Load configuration from a file.
 * If filename is NULL, searches for config in the following order:
 * 1. $XDG_CONFIG_HOME/belvedere/config
 * 2. $HOME/.config/belvedere/config
 * 3. /etc/belvedere/config
 *
 * @param filename Optional path to config file. If NULL, uses XDG paths.
 * @param config Pointer to config_t structure to populate
 * @return true if config was loaded successfully, false otherwise
 */
bool load_config(const char *filename, config_t *config);

/**
 * Get the command string for a given key on a device.
 *
 * @param config Pointer to loaded configuration
 * @param vendor Device vendor ID
 * @param product Device product ID
 * @param keycode Key code to look up
 * @return Command string if found, NULL otherwise
 */
const char *get_command_for_key(const config_t *config, uint16_t vendor, uint16_t product, uint16_t keycode);
