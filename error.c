#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void PrintError(char *message, ...)
{
	va_list args;
	va_start(args, message);

	vprintf(message, args);

	va_end(args);

	exit(1);
}
