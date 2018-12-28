#include <stdio.h>

#include "memory_stream.h"
#include "smps2asm2bin.h"

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	MemoryStream *output_stream = MemoryStream_Create(0x100, false);

	SMPS2ASM2BIN("song.asm", output_stream, 2, 0x1380);

	FILE *out_file = fopen("song.bin", "wb");
	MemoryStream_SetPosition(output_stream, 0, MEMORYSTREAM_END);
	fwrite(MemoryStream_GetBuffer(output_stream), 1, MemoryStream_GetPosition(output_stream), out_file);
	fclose(out_file);

	return 0;
}
