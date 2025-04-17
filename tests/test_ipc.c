#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <string.h>
#include <uv.h>
#include "../include/ipc.h"
#include "mock_debug.h"  // Include mock debug header

// Test fixtures
static ipc_server_t server;
static uv_loop_t loop;

// Setup function called before each test
static int ipc_test_setup(void) {
    uv_loop_init(&loop);
    return 0;
}

// Teardown function called after each test
static int ipc_test_teardown(void) {
    ipc_cleanup(&server);
    uv_loop_close(&loop);
    return 0;
}

// Test initialization
static void test_ipc_init(void) {
    int result = ipc_init(&server, &loop);
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(server.is_running, 0);
}

// Test server start/stop
static void test_ipc_start_stop(void) {
    int result = ipc_init(&server, &loop);
    CU_ASSERT_EQUAL(result, 0);

    result = ipc_start(&server);
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_EQUAL(server.is_running, 1);

    ipc_stop(&server);
    CU_ASSERT_EQUAL(server.is_running, 0);
}

// Test command processing
static void test_ipc_process_command(void) {
    int result = ipc_init(&server, &loop);
    CU_ASSERT_EQUAL(result, 0);

    char response[256];
    result = ipc_process_command(&server, "status", response, sizeof(response));
    CU_ASSERT_EQUAL(result, 0);
    CU_ASSERT_STRING_EQUAL(response, "OK");
}

// Register test suite
CU_TestInfo ipc_tests[] = {
    {"test_ipc_init", test_ipc_init},
    {"test_ipc_start_stop", test_ipc_start_stop},
    {"test_ipc_process_command", test_ipc_process_command},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ipc_suite = {
    "IPC Suite",
    ipc_test_setup,
    ipc_test_teardown,
    NULL,
    NULL,
    ipc_tests
};

int main(void) {
    // Initialize CUnit test registry
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    // Add suite to registry
    if (CUE_SUCCESS != CU_register_suites(&ipc_suite)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Run tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Get number of failures
    unsigned int num_failures = CU_get_number_of_failures();

    // Clean up
    CU_cleanup_registry();

    // Return 0 if all tests passed, 1 otherwise
    return num_failures == 0 ? 0 : 1;
}