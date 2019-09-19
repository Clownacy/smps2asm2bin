#include "smps2asm2bin.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "dictionary.h"
#include "error.h"
#include "instruction.h"
#include "memory_stream.h"

typedef struct DelayedInstruction
{
	struct DelayedInstruction *next;

	char *instruction;
	unsigned int arg_count;
	char **arg_array;
	size_t output_position;
} DelayedInstruction;

static DelayedInstruction *delayed_instruction_list_head;

static void ParseLine(char *line)
{
	// Remove comments
	char *comment_start = strchr(line, ';');
	if (comment_start)
		*comment_start = '\0';

	// Look for a label
	const size_t size_of_label = strcspn(line, " \t:");
	size_t size_of_whitespace = strspn(line + size_of_label, " \t:");

	if (size_of_label != 0)
	{
		// We found a label!
		line[size_of_label] = '\0';
		HandleLabel(line);
	}

	line += size_of_label + size_of_whitespace;

	// Look for an instruction
	const size_t size_of_instruction = strcspn(line, " \t");

	if (size_of_instruction != 0)
	{
		size_t size_of_whitespace = strspn(line + size_of_instruction, " \t");

		// We found an instruction!
		line[size_of_instruction] = '\0';
		char *instruction = line;

		// Look for the instruction's arguments
		line += size_of_instruction + size_of_whitespace;

		unsigned int arg_count = 0;
		char **arg_array = NULL;

		for (;;)
		{
			const size_t size_of_arg = strcspn(line, " \t,");
			const size_t size_of_whitespace = strspn(line + size_of_arg, " \t,");
			const bool is_last_arg = strchr(line, ',') == NULL;

			if (size_of_arg != 0)
			{
				// We found an argument!
				line[size_of_arg] = '\0';
				arg_array = realloc(arg_array, sizeof(*arg_array) * (arg_count + 1));
				arg_array[arg_count++] = line;

				if (is_last_arg)
					break;

				line += size_of_arg + size_of_whitespace;
			}
			else
				break;

		}

		const size_t output_position = MemoryStream_GetPosition(output_stream);

		// Now that we've gathered-up the instruction and arguments in a nice format
		// that we can process, pass them to the function that actually parses them
		undefined_symbol = NULL;
		HandleInstruction(instruction, arg_count, arg_array);

		if (undefined_symbol != 0)
		{
			// Instructions that reference undefined symbols can't be
			// fully-outputted yet, so stick them in a list for later
			DelayedInstruction *delayed_instruction = malloc(sizeof(*delayed_instruction));
			delayed_instruction->next = delayed_instruction_list_head;
			delayed_instruction_list_head = delayed_instruction;

			delayed_instruction->instruction = instruction;
			delayed_instruction->arg_count = arg_count;
			delayed_instruction->arg_array = arg_array;
			delayed_instruction->output_position = output_position;
		}
		else
		{
			free(arg_array);
		}
	}
}

bool SMPS2ASM2BIN(char *file_name, MemoryStream *p_output_stream, unsigned int p_target_driver, size_t p_file_offset)
{
	bool success = false;

	output_stream = p_output_stream;
	target_driver = p_target_driver;
	file_offset = p_file_offset;

	FILE *in_file = fopen(file_name, "rb");

	if (in_file != NULL)
	{
		// Read the input file into a string buffer
		fseek(in_file, 0, SEEK_END);
		const size_t in_file_size = ftell(in_file);
		rewind(in_file);
		char *in_file_buffer = malloc(in_file_size + 1);
		fread(in_file_buffer, 1, in_file_size, in_file);
		fclose(in_file);

		in_file_buffer[in_file_size] = '\0';

		FillDefaultDictionary();

		size_t index = 0;

		// Feed each line into the ParseLine function
		for (;;)
		{
			const size_t size_of_line = strcspn(in_file_buffer + index, "\r\n");	// Preeeeeetty sure this won't work if the file doesn't end with a newline

			in_file_buffer[index + size_of_line] = '\0';

			ParseLine(in_file_buffer + index);

			if (index + size_of_line == in_file_size)
				break;

			index += size_of_line + 1;
			index += strspn(in_file_buffer + index, "\r\n");

			if (error)
				goto fail;
		}

		// Once the entire file is processed, finish any instructions that had to be delayed because of undefined symbols
		for (DelayedInstruction *instruction = delayed_instruction_list_head; instruction != NULL; instruction = instruction->next)
		{
			undefined_symbol = NULL;
			MemoryStream_SetPosition(output_stream, instruction->output_position, MEMORYSTREAM_START);
			HandleInstruction(instruction->instruction, instruction->arg_count, instruction->arg_array);

			if (undefined_symbol != NULL)
				PrintError("Error: symbol '%s' undefined\n", undefined_symbol);

			if (error)
				goto fail;
		}

		success = true;

		fail:;

		// Delete the list of delayed instructions
		DelayedInstruction *entry = delayed_instruction_list_head;
		while (entry != NULL)
		{
			DelayedInstruction *next_entry = entry->next;
			free(entry);
			entry = next_entry;
		}

		delayed_instruction_list_head = NULL;

		ClearDictionary();
		free(in_file_buffer);

	}

	return success;
}
