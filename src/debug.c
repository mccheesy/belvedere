#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "../include/debug.h"

bool debug_enabled = false;

void debug(const char *format, ...) {
    if (!debug_enabled) return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void debugf(FILE *stream, const char *format, ...) {
    va_list args;

    if (debug_enabled) {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    va_start(args, format);
    vfprintf(stream, format, args);
    va_end(args);
}
