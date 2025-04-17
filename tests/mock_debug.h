#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

// Mock debug functions for testing
extern bool debug_enabled;

void debug(const char *format, ...);
void debugf(FILE *stream, const char *format, ...);

// Mock implementation that does nothing
#define MOCK_DEBUG 1