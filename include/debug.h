#pragma once

#include <stdio.h>
#include <stdbool.h>

extern bool debug_enabled;

void debug(const char *format, ...);
void debugf(FILE *stream, const char *format, ...);