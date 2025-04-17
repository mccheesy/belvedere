#include "mock_debug.h"

bool debug_enabled = false;

void debug(const char *format, ...) {
    // Mock implementation - do nothing
}

void debugf(FILE *stream, const char *format, ...) {
    // Mock implementation - do nothing
}