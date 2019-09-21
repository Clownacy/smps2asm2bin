#include <stdio.h>

#include "memory_stream.h"
#include "smps2asm2bin.h"

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	MemoryStream *output_stream = MemoryStream_Create(0x100, true);

	if (!SMPS2ASM2BIN(argc > 1 ? argv[1] : "song.asm", output_stream, 2, 0x1380))
	{
		MemoryStream_Destroy(output_stream);
		puts("Aww man, I fucked up :(");
		return 1;
	}
	else
	{
		FILE *out_file = fopen("song.bin", "wb");
		MemoryStream_SetPosition(output_stream, 0, MEMORYSTREAM_END);
		fwrite(MemoryStream_GetBuffer(output_stream), 1, MemoryStream_GetPosition(output_stream), out_file);
		fclose(out_file);

		MemoryStream_Destroy(output_stream);

		return 0;
	}
}
