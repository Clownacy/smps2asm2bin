#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "memory_stream.h"

bool SMPS2ASM2BIN(char *file_name, MemoryStream *p_output_stream, unsigned int p_target_driver, size_t p_file_offset);
