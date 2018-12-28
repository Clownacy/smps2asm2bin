CFLAGS := -O2 -s -Wall -Wextra

a: main.c common.c dictionary.c error.c instruction.c memory_stream.c smps2asm2bin.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)
