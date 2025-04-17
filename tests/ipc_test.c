#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../include/ipc.h"
#include "../include/config.h"

// Test fixtures
static uv_loop_t* loop;
static config_t test_config;

// Setup and teardown functions
static int ipc_test_init(void) {
    // Initialize libuv loop
    loop = uv_default_loop();
    if (!loop) {
        return -1;
    }

    // Initialize test configuration
    memset(&test_config, 0, sizeof(config_t));
    test_config.device_count = 2;

    return 0;
}

static int ipc_test_cleanup(void) {
    // Cleanup IPC server
    ipc_cleanup();

    // Run loop to process pending events
    uv_run(loop, UV_RUN_NOWAIT);

    return 0;
}

// Test functions
static void test_ipc_init(void) {
    // Test successful initialization
    CU_ASSERT_TRUE(ipc_init(loop));

    // Test that socket file exists
    CU_ASSERT_EQUAL(access("/tmp/belvedere.sock", F_OK), 0);

    // Test that server is running
    CU_ASSERT_TRUE(ipc_init(loop)); // Should return true if already running
}

static void test_ipc_cleanup(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Cleanup server
    ipc_cleanup();

    // Test that socket file is removed
    CU_ASSERT_EQUAL(access("/tmp/belvedere.sock", F_OK), -1);
}

static void test_ipc_process_command_reload(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Test reload command
    ipc_response_t response = ipc_process_command("reload");

    // Since we don't have a real config file in tests, this should fail
    CU_ASSERT_FALSE(response.success);
    CU_ASSERT_STRING_EQUAL(response.message, "Failed to reload configuration");
}

static void test_ipc_process_command_status(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Test status command
    ipc_response_t response = ipc_process_command("status");

    // Should return success with device count
    CU_ASSERT_TRUE(response.success);
    CU_ASSERT_STRING_CONTAINS(response.message, "Belvedere is running with");
}

static void test_ipc_process_command_unknown(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Test unknown command
    ipc_response_t response = ipc_process_command("unknown_command");

    // Should return failure with error message
    CU_ASSERT_FALSE(response.success);
    CU_ASSERT_STRING_CONTAINS(response.message, "Unknown command");
}

static void test_ipc_process_command_null(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Test null command
    ipc_response_t response = ipc_process_command(NULL);

    // Should return failure with error message
    CU_ASSERT_FALSE(response.success);
    CU_ASSERT_STRING_EQUAL(response.message, "No command provided");
}

// Client test functions
static void test_ipc_client_connection(void) {
    // Initialize server
    CU_ASSERT_TRUE(ipc_init(loop));

    // Create a client pipe
    uv_pipe_t client;
    uv_pipe_init(loop, &client, 0);

    // Connect to server
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/belvedere.sock", sizeof(addr.sun_path) - 1);

    // Connect and run loop to process connection
    uv_connect_t connect;
    int r = uv_pipe_connect(&connect, &client, (const struct sockaddr*)&addr, NULL);
    CU_ASSERT_EQUAL(r, 0);

    // Run loop to process connection
    uv_run(loop, UV_RUN_NOWAIT);

    // Cleanup
    uv_close((uv_handle_t*)&client, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

// Test suite registration
CU_TestInfo ipc_tests[] = {
    {"IPC Init", test_ipc_init},
    {"IPC Cleanup", test_ipc_cleanup},
    {"IPC Process Command - Reload", test_ipc_process_command_reload},
    {"IPC Process Command - Status", test_ipc_process_command_status},
    {"IPC Process Command - Unknown", test_ipc_process_command_unknown},
    {"IPC Process Command - Null", test_ipc_process_command_null},
    {"IPC Client Connection", test_ipc_client_connection},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ipc_suite = {
    "IPC Suite",
    ipc_test_init,
    ipc_test_cleanup,
    NULL,
    NULL,
    ipc_tests
};

// Register suites
void ipc_suite_register(void) {
    CU_register_suite(&ipc_suite);
}