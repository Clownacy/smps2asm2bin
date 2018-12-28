#pragma once

#include <stdarg.h>
#include <stdbool.h>

extern bool error;

void PrintError(char *message, ...);
