CFLAGS := -O2 -s -std=c99 -fno-ident -flto -Wall -Wextra -pedantic

smps2asm2bin: main.c common.c dictionary.c error.c instruction.c memory_stream.c smps2asm2bin.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)
