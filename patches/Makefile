CC      ?= clang
LD      ?= ld.lld
OBJCOPY ?= llvm-objcopy

CFLAGS  := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg \
           -mno-check-zero-division -fno-common -fvisibility=hidden
CPPFLAGS := -nostdinc -DF3DEX_GBI_2 -D_LANGUAGE_C -DMIPS \
            -I dummy_headers \
            -I ../lib/pokemonsnap-decomp/include \
            -I ../lib/rt64/include \
            -I ../include

LDFLAGS := -nostdlib -T patches.ld -T syms.ld \
           --unresolved-symbols=ignore-all --emit-relocs

SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

all: patches.elf

patches.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c patches.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) patches.elf patches.bin

.PHONY: all clean
