#include <stdio.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// Test suite declarations
extern void ipc_suite_register(void);

int main(void) {
    // Initialize CUnit test registry
    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    // Register test suites
    ipc_suite_register();

    // Run all tests
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Clean up
    CU_cleanup_registry();
    return CU_get_error();
}