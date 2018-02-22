CCFLAGS=-m32 -Wall -Wextra -Werror -fno-stack-protector -nostdinc -fno-builtin
ASFLAGS=-m32
OUTPUT=COM1

UNAME_S!=uname -s
LIBGCC!=$(CC) ${CCFLAGS} -print-libgcc-file-name

LDEMULATION=elf_i386
.if ${UNAME_S:MFreeBSD}
LDEMULATION=elf_i386_fbsd
.endif

LDFLAGS=-m${LDEMULATION} -T link.ld

.PHONY: all
all: mmap.elf modules.elf

.PHONY: clean
clean:
	rm -f start.o mmap.o libc.o test-multiboot.o
	rm -f mmap.elf
	rm -f modules.elf

mmap.elf: start.o mmap.o libc.o
	$(LD) $(LDFLAGS) -o $@ $> $(LIBGCC)

modules.elf: start.o modules.o libc.o
	$(LD) $(LDFLAGS) -o $@ $> $(LIBGCC)

.c.o:
	$(CC) -DOUTPUT=$(OUTPUT) $(CCFLAGS) -c -o $@ $<

.S.o:
	$(CC) $(ASFLAGS) -c -o $@ $<
