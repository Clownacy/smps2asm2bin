#pragma once

#include <stddef.h>

#include "memory_stream.h"

extern size_t file_offset;
extern unsigned int target_driver;

void FillDefaultDictionary(void);
void HandleLabel(char *label);
void HandleInstruction(char *opcode, unsigned int arg_count, char *arg_array[]);
