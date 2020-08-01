#include <stdio.h>

#include "memory_stream.h"
#include "smps2asm2bin.h"

#include <stdlib.h>
#include <string.h>

/* Program usage */
const char * usageMessageStr = 
	"USAGE:\n"
	"	%s [-v driver_version] [-o hex_offset] in_file_path [out_file_path]\n"	// "%s" should substitute for argv[0]
	"\n"
	"OPTIONS:\n"
	"	-v driver_version\n"
	"		Specifies the target driver version:\n"
	"			1 = Sonic 1 (default)\n"
	"			2 = Sonic 2\n"
	"			3 = Sonic 3 & Knuckles\n"
	"\n"
	"	-o hex_offset\n"
	"		Base offset for the binary file (hexadecimal).\n"
	"\n";

/*
 * Helper function to parse arguments
 */
int parseArgs(
	int argc, 
	char *argv[], 
	const char ** in_file_path_ptr, 		// memory address for "in_file_path" argument
	const char ** out_file_path_ptr, 		// memory address for "out_file_path" argument
	unsigned int * target_driver_ptr,		// memory address for "target_driver_ptr" argument
	size_t * file_offset_ptr				// memory address for "file_offset_ptr"argument
) {

	int arg_index = 1;		// Tracks index of currently processed argument

	/* Process options (if available) */
	for (; arg_index < argc && argv[arg_index][0] == '-'; arg_index += 2) {
		const char * option_name = argv[arg_index];
		const char * option_raw_value = argv[arg_index+1];

		if (strcmp(option_name, "-v") == 0) {
			*target_driver_ptr = (unsigned int)strtol(option_raw_value, NULL, 10);
		}
		else if (strcmp(option_name, "-o") == 0) {
			*file_offset_ptr = (size_t)strtol(option_raw_value, NULL, 0x10);
		}
		else {
			fprintf(stderr, "ERROR: Unrecognized option \"%s\"\n", option_name);
			return -1;
		}
	}

	/* Process "in_file_path" argument */
	if (arg_index >= argc) {
		fprintf(stderr, "ERROR: Expected \"in_file_path\" after options\n");
		return -2;
	}

	*in_file_path_ptr = argv[arg_index++];

	/* Process "out_file_path" argument */
	if (arg_index >= argc) {
		const char * extension = ".bin";
		const size_t buffer_length = strlen(*in_file_path_ptr) + strlen(extension) + 1;
		char * buffer = malloc(buffer_length);

		snprintf(buffer, buffer_length, "%s%s", *in_file_path_ptr, extension);

		*out_file_path_ptr = (const char*)buffer;
	}
	else {
		*out_file_path_ptr = argv[arg_index++];
	}

	return 0;
}

int main(int argc, char *argv[])
{
	/* When called without arguments, print usage */
	if (argc < 2) {
		fprintf(stderr, usageMessageStr, argv[0]);
		return 1;
	}

	MemoryStream *output_stream = MemoryStream_Create(true);

	/* Parse input arguments */
	const char * in_file_path = NULL;
	const char * out_file_path = NULL;
	unsigned int target_driver = 1;
	size_t file_offset = 0;

	int parseResult = parseArgs(argc, argv, &in_file_path, &out_file_path, &target_driver, &file_offset);

	if (parseResult != 0) {
		fprintf(stderr, "Error during arguments parsing, unable to continue.\n");
		return parseResult;
	}

	/* Read file and process it */
	if (!SMPS2ASM2BIN(in_file_path, output_stream, target_driver, file_offset))
	{
		MemoryStream_Destroy(output_stream);
		fprintf(stderr, "Processing of \"%s\" file halted due to an error.\n", in_file_path);
		return 1;
	}

	/* Write down the output */
	FILE *out_file = fopen(out_file_path, "wb");

	MemoryStream_SetPosition(output_stream, 0, MEMORYSTREAM_END);
	fwrite(MemoryStream_GetBuffer(output_stream), 1, MemoryStream_GetPosition(output_stream), out_file);
	fclose(out_file);

	MemoryStream_Destroy(output_stream);

	return 0;
}
