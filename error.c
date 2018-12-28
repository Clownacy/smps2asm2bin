#include "error.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool error;

void PrintError(char *message, ...)
{
	va_list args;
	va_start(args, message);

	vprintf(message, args);

	va_end(args);

	error = true;
}
