// config.c
#include "../include/config.h"

#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/debug.h"

static char* get_config_path(void)
{
    static char config_path[PATH_MAX];
    const char* xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");

    // Try XDG_CONFIG_HOME first
    if (xdg_config_home && *xdg_config_home)
    {
        snprintf(config_path, sizeof(config_path), "%s/belvedere/config", xdg_config_home);
        if (access(config_path, F_OK) == 0)
        {
            return config_path;
        }
    }

    // Fall back to $HOME/.config
    if (home && *home)
    {
        snprintf(config_path, sizeof(config_path), "%s/.config/belvedere/config", home);
        if (access(config_path, F_OK) == 0)
        {
            return config_path;
        }
    }

    // Fall back to /etc/belvedere/config
    snprintf(config_path, sizeof(config_path), "/etc/belvedere/config");
    if (access(config_path, F_OK) == 0)
    {
        return config_path;
    }

    return NULL;
}

static char* trim(char* str)
{
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
    debug("Trimmed string: '%s'\n", str);
    return str;
}

bool load_config(const char* filename, config_t* config)
{
    const char* config_path = filename ? filename : get_config_path();
    if (!config_path)
    {
        debug("No configuration file found\n");
        return false;
    }

    FILE* file = fopen(config_path, "r");
    if (!file)
    {
        debug("Failed to open configuration file: %s\n", config_path);
        return false;
    }

    debug("Loading configuration from: %s\n", config_path);

    char line[256];
    device_config_t* current = NULL;
    config->device_count = 0;
    config->setleds_path[0] = '\0';
    bool in_general_section = false;

    config->monitored_keycodes_count = 0;
    memset(config->monitored_keycodes, 0, sizeof(config->monitored_keycodes_count));

    while (fgets(line, sizeof(line), file))
    {
        char* trimmed = trim(line);
        if (*trimmed == '\0' || *trimmed == '#')
            continue;

        if (*trimmed == '[')
        {
            if (strcasecmp(trimmed, "[general]") == 0)
            {
                in_general_section = true;
                current = NULL;
                continue;
            }
            else
            {
                in_general_section = false;
                if (config->device_count >= MAX_CONFIG_DEVICES)
                    continue;
                unsigned int vid, pid;
                if (sscanf(trimmed, "[%x/%x]", &vid, &pid) == 2)
                {
                    current = &config->devices[config->device_count++];
                    current->vendor = vid;
                    current->product = pid;
                    current->binding_count = 0;
                    current->target[0] = '\0';
                }
                else
                {
                    current = NULL;
                }
            }
        }
        else if (in_general_section)
        {
            char* eq = strchr(trimmed, '=');
            if (!eq)
                continue;
            *eq = '\0';
            char* key = trim(trimmed);
            char* val = trim(eq + 1);
            if (strcasecmp(key, "setleds") == 0)
            {
                strncpy(config->setleds_path, val, sizeof(config->setleds_path) - 1);
                config->setleds_path[sizeof(config->setleds_path) - 1] = '\0';
            }
            else if (strcasecmp(key, "monitored_keycodes") == 0)
            {
                // Parse comma-separated keycodes (supports decimal and hex)
                char* token = strtok(val, ",");
                while (token && config->monitored_keycodes_count < MAX_MONITORED_KEYCODES)
                {
                    int keycode;
                    if (strncmp(token, "0x", 2) == 0 || strncmp(token, "0X", 2) == 0)
                    {
                        // Parse hexadecimal keycode
                        keycode = strtol(token + 2, NULL, 16);
                    }
                    else
                    {
                        // Parse decimal keycode
                        keycode = atoi(token);
                    }

                    if (keycode >= 0 && keycode <= 0xFFFF)
                    {  // Allow full 16-bit range
                        config->monitored_keycodes[config->monitored_keycodes_count++] =
                            (uint32_t)keycode;
                        debug("Parsed monitored keycode: 0x%04x (%d)\n", keycode, keycode);
                    }
                    else
                    {
                        debugf(stderr,
                               "Invalid keycode: 0x%04x in monitored_keycodes. Must be between 0 "
                               "and 0xFFFF.\n",
                               keycode);
                    }
                    token = strtok(NULL, ",");
                }
            }
        }
        else if (current)
        {
            char* eq = strchr(trimmed, '=');
            if (!eq)
                continue;
            *eq = '\0';
            char* key = trim(trimmed);
            char* val = trim(eq + 1);

            if (strcasecmp(key, "target") == 0)
            {
                strncpy(current->target, val, sizeof(current->target) - 1);
                current->target[sizeof(current->target) - 1] = '\0';
            }
            else
            {
                if (current->binding_count >= MAX_BINDINGS)
                    continue;
                if (strlen(val) < 2)
                    continue;  // Require at least mode+led

                key_binding_t* binding = &current->bindings[current->binding_count++];
                int keycode;
                if (strncmp(key, "0x", 2) == 0 || strncmp(key, "0X", 2) == 0)
                {
                    // Parse hexadecimal keycode
                    keycode = strtol(key + 2, NULL, 16);
                }
                else
                {
                    // Parse decimal keycode
                    keycode = atoi(key);
                }
                binding->keycode = (uint16_t)keycode;
                binding->mode = val[0];
                strncpy(binding->led, val + 1, sizeof(binding->led) - 1);
                binding->led[sizeof(binding->led) - 1] = '\0';
            }
        }
    }

    fclose(file);

    if (config->setleds_path[0] == '\0')
    {
        strncpy(config->setleds_path, DEFAULT_SETLEDS_PATH, sizeof(config->setleds_path) - 1);
        config->setleds_path[sizeof(config->setleds_path) - 1] = '\0';
    }

    // Process each device
    for (size_t i = 0; i < config->device_count; i++)
    {
        device_config_t* dev = &config->devices[i];
        debug("Processing device %zu: VID=0x%04x, PID=0x%04x\n", i, dev->vendor, dev->product);

        // Process each binding for this device
        for (size_t j = 0; j < dev->binding_count; j++)
        {
            key_binding_t* binding = &dev->bindings[j];
            debug("  Binding %zu: keycode=0x%04x, led=%s, mode=%c\n", j, binding->keycode,
                  binding->led, binding->mode);
        }
    }

    return true;
}

const char* get_command_for_key(const config_t* config, uint16_t vendor, uint16_t product,
                                uint16_t keycode)
{
    static char cmd_buffer[256];  // Static buffer for the command string

    // Find matching device
    for (size_t i = 0; i < config->device_count; i++)
    {
        const device_config_t* dev = &config->devices[i];
        if (dev->vendor == vendor && dev->product == product)
        {
            // Find matching binding
            for (size_t j = 0; j < dev->binding_count; j++)
            {
                const key_binding_t* binding = &dev->bindings[j];
                if (binding->keycode == keycode)
                {
                    // Construct the full command string
                    snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %c%s", config->setleds_path,
                             binding->mode, binding->led);
                    return cmd_buffer;
                }
            }
            break;  // Device found but no matching binding
        }
    }
    return NULL;  // No matching device or binding
}
