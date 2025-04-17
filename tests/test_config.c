#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <limits.h>     // for PATH_MAX
#include <sys/stat.h>   // for mkdir
#include <unistd.h>     // for mkdtemp, unlink, rmdir
#include "../include/config.h"
#include "../include/debug.h"

// Global configuration
config_t config = {0};

// Test cases
void test_load_config_basic(void) {
    // Save original environment variables
    char *old_xdg_config = getenv("XDG_CONFIG_HOME");
    char *old_home = getenv("HOME");

    // Make copies of the original values
    char *xdg_config_copy = old_xdg_config ? strdup(old_xdg_config) : NULL;
    char *home_copy = old_home ? strdup(old_home) : NULL;

    // Create a temporary test directory
    char temp_dir[] = "/tmp/belvedere_test_XXXXXX";
    char *test_dir = mkdtemp(temp_dir);
    if (!test_dir) {
        // Clean up and return if we can't create temp directory
        if (xdg_config_copy) free(xdg_config_copy);
        if (home_copy) free(home_copy);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Set up test environment
    if (setenv("XDG_CONFIG_HOME", test_dir, 1) != 0 ||
        setenv("HOME", "/tmp/nonexistent", 1) != 0) {
        // Clean up and return if we can't set environment variables
        rmdir(test_dir);
        if (xdg_config_copy) free(xdg_config_copy);
        if (home_copy) free(home_copy);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Create test config file in the temporary directory
    char test_config_dir[PATH_MAX];
    snprintf(test_config_dir, sizeof(test_config_dir), "%s/belvedere", test_dir);
    if (mkdir(test_config_dir, 0755) != 0) {
        // Clean up and return if we can't create config directory
        rmdir(test_dir);
        if (xdg_config_copy) free(xdg_config_copy);
        if (home_copy) free(home_copy);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    char test_config_file[PATH_MAX];
    snprintf(test_config_file, sizeof(test_config_file), "%s/config", test_config_dir);
    FILE *f = fopen(test_config_file, "w");
    if (!f) {
        // Clean up and return if we can't create config file
        rmdir(test_config_dir);
        rmdir(test_dir);
        if (xdg_config_copy) free(xdg_config_copy);
        if (home_copy) free(home_copy);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Write test configuration
    fprintf(f, "[0x5043/0x54a3]\n");
    fprintf(f, "target = *\n");
    fprintf(f, "111 = +caps\n");
    fclose(f);

    // Test loading a valid configuration
    config_t test_config = {0};
    CU_ASSERT(load_config(NULL, &test_config) == true);
    CU_ASSERT_EQUAL(test_config.device_count, 1);
    CU_ASSERT_EQUAL(test_config.devices[0].vendor, 0x5043);
    CU_ASSERT_EQUAL(test_config.devices[0].product, 0x54a3);
    CU_ASSERT_EQUAL(test_config.devices[0].binding_count, 1);

    // Test loading a non-existent file
    CU_ASSERT(load_config("nonexistent.ini", &test_config) == false);

    // Update config file with general section
    f = fopen(test_config_file, "w");
    if (!f) {
        // Clean up and return if we can't update config file
        unlink(test_config_file);
        rmdir(test_config_dir);
        rmdir(test_dir);
        if (xdg_config_copy) free(xdg_config_copy);
        if (home_copy) free(home_copy);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }
    fprintf(f, "[general]\n");
    fprintf(f, "setleds = /test/path/setleds\n");
    fclose(f);

    // Test loading from XDG path with general section
    CU_ASSERT(load_config(NULL, &test_config) == true);
    CU_ASSERT_STRING_EQUAL(test_config.setleds_path, "/test/path/setleds");

    // Clean up test files
    unlink(test_config_file);
    rmdir(test_config_dir);
    rmdir(test_dir);

    // Restore environment variables
    if (xdg_config_copy) {
        setenv("XDG_CONFIG_HOME", xdg_config_copy, 1);
        free(xdg_config_copy);
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }
    if (home_copy) {
        setenv("HOME", home_copy, 1);
        free(home_copy);
    } else {
        unsetenv("HOME");
    }

    // Verify restoration by comparing values instead of pointers
    char *current_xdg_config = getenv("XDG_CONFIG_HOME");
    char *current_home = getenv("HOME");
    if (old_xdg_config) {
        CU_ASSERT_STRING_EQUAL(current_xdg_config, old_xdg_config);
    } else {
        CU_ASSERT_PTR_NULL(current_xdg_config);
    }
    if (old_home) {
        CU_ASSERT_STRING_EQUAL(current_home, old_home);
    } else {
        CU_ASSERT_PTR_NULL(current_home);
    }
}

void test_load_config_sections(void) {
    config_t test_config = {0};

    // Create a temporary test directory
    char temp_dir[] = "/tmp/belvedere_test_XXXXXX";
    char *test_dir = mkdtemp(temp_dir);
    if (!test_dir) {
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Create test config file
    char test_config_file[PATH_MAX];
    snprintf(test_config_file, sizeof(test_config_file), "%s/test_config_sections.ini", test_dir);
    FILE *f = fopen(test_config_file, "w");
    if (!f) {
        rmdir(test_dir);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    fprintf(f, "[general]\n");
    fprintf(f, "setleds = /custom/path/setleds\n");
    fprintf(f, "monitored_keycodes = 0x1234,5678,0xABCD\n");
    fprintf(f, "\n");
    fprintf(f, "[0x5043/0x54a3]\n");
    fprintf(f, "target = *\n");
    fprintf(f, "0x6F = +caps\n");
    fprintf(f, "\n");
    fprintf(f, "[0x0483/0x5740]\n");
    fprintf(f, "target = device2\n");
    fprintf(f, "111 = -num\n");
    fclose(f);

    CU_ASSERT(load_config(test_config_file, &test_config) == true);

    // Verify general section
    CU_ASSERT_STRING_EQUAL(test_config.setleds_path, "/custom/path/setleds");
    CU_ASSERT_EQUAL(test_config.monitored_keycodes_count, 3);
    CU_ASSERT_EQUAL(test_config.monitored_keycodes[0], 0x1234);
    CU_ASSERT_EQUAL(test_config.monitored_keycodes[1], 5678);
    CU_ASSERT_EQUAL(test_config.monitored_keycodes[2], 0xABCD);

    // Verify device sections
    CU_ASSERT_EQUAL(test_config.device_count, 2);

    // First device
    CU_ASSERT_EQUAL(test_config.devices[0].vendor, 0x5043);
    CU_ASSERT_EQUAL(test_config.devices[0].product, 0x54a3);
    CU_ASSERT_STRING_EQUAL(test_config.devices[0].target, "*");
    CU_ASSERT_EQUAL(test_config.devices[0].binding_count, 1);
    CU_ASSERT_EQUAL(test_config.devices[0].bindings[0].keycode, 0x6F);
    CU_ASSERT_EQUAL(test_config.devices[0].bindings[0].mode, '+');
    CU_ASSERT_STRING_EQUAL(test_config.devices[0].bindings[0].led, "caps");

    // Second device
    CU_ASSERT_EQUAL(test_config.devices[1].vendor, 0x0483);
    CU_ASSERT_EQUAL(test_config.devices[1].product, 0x5740);
    CU_ASSERT_STRING_EQUAL(test_config.devices[1].target, "device2");
    CU_ASSERT_EQUAL(test_config.devices[1].binding_count, 1);
    CU_ASSERT_EQUAL(test_config.devices[1].bindings[0].keycode, 111);
    CU_ASSERT_EQUAL(test_config.devices[1].bindings[0].mode, '-');
    CU_ASSERT_STRING_EQUAL(test_config.devices[1].bindings[0].led, "num");

    // Clean up
    unlink(test_config_file);
    rmdir(test_dir);
}

void test_load_config_limits(void) {
    config_t test_config = {0};

    // Create a temporary test directory
    char temp_dir[] = "/tmp/belvedere_test_XXXXXX";
    char *test_dir = mkdtemp(temp_dir);
    if (!test_dir) {
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Create test config file
    char test_config_file[PATH_MAX];
    snprintf(test_config_file, sizeof(test_config_file), "%s/test_config_limits.ini", test_dir);
    FILE *f = fopen(test_config_file, "w");
    if (!f) {
        rmdir(test_dir);
        CU_ASSERT_FATAL(0);  // Fail the test
        return;
    }

    // Add more devices than MAX_DEVICES
    for (int i = 0; i < MAX_DEVICES + 2; i++) {
        fprintf(f, "[0x%04x/0x%04x]\n", i, i);
        fprintf(f, "target = device%d\n", i);
        // Add more bindings than MAX_BINDINGS
        for (int j = 0; j < MAX_BINDINGS + 2; j++) {
            fprintf(f, "%d = +caps\n", j);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    CU_ASSERT(load_config(test_config_file, &test_config) == true);

    // Verify limits are respected
    CU_ASSERT_EQUAL(test_config.device_count, MAX_DEVICES);
    for (size_t i = 0; i < test_config.device_count; i++) {
        CU_ASSERT_EQUAL(test_config.devices[i].binding_count, MAX_BINDINGS);
    }

    // Clean up
    unlink(test_config_file);
    rmdir(test_dir);
}

void test_get_command_for_key(void) {
    // Set up test configuration
    config_t test_config = {0};
    test_config.device_count = 1;
    test_config.devices[0].vendor = 0x5043;
    test_config.devices[0].product = 0x54a3;

    // Test different LED modes
    const char* modes[] = {"+", "-", "^"};
    const char* leds[] = {"caps", "num", "scroll"};

    strncpy(test_config.setleds_path, "/usr/local/bin/setleds", sizeof(test_config.setleds_path) - 1);

    for (size_t i = 0; i < 3; i++) {
        test_config.devices[0].bindings[i].keycode = 111 + i;
        test_config.devices[0].bindings[i].mode = modes[i][0];
        strncpy(test_config.devices[0].bindings[i].led, leds[i], sizeof(test_config.devices[0].bindings[i].led) - 1);
    }
    test_config.devices[0].binding_count = 3;

    // Test each mode/LED combination
    for (size_t i = 0; i < 3; i++) {
        const char* cmd = get_command_for_key(&test_config, 0x5043, 0x54a3, 111 + i);
        char expected[256];
        snprintf(expected, sizeof(expected), "/usr/local/bin/setleds %s%s", modes[i], leds[i]);
        CU_ASSERT_PTR_NOT_NULL(cmd);
        CU_ASSERT_STRING_EQUAL(cmd, expected);
    }

    // Test invalid cases
    CU_ASSERT_PTR_NULL(get_command_for_key(&test_config, 0x5043, 0x54a3, 999));  // Invalid keycode
    CU_ASSERT_PTR_NULL(get_command_for_key(&test_config, 0x1234, 0x5678, 111));  // Invalid device
}

int main(void) {
    // Initialize CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // Create test suite
    CU_pSuite pSuite = CU_add_suite("Config Tests", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add tests to suite
    if ((NULL == CU_add_test(pSuite, "test_load_config_basic", test_load_config_basic)) ||
        (NULL == CU_add_test(pSuite, "test_load_config_sections", test_load_config_sections)) ||
        (NULL == CU_add_test(pSuite, "test_load_config_limits", test_load_config_limits)) ||
        (NULL == CU_add_test(pSuite, "test_get_command_for_key", test_get_command_for_key))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run all tests using the basic interface
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Clean up
    CU_cleanup_registry();
    return CU_get_error();
}