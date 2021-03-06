OBJCOPY?=objcopy

LIBELF=     -lelf
LIBATF_C=/usr/local/lib/libatf-c.a

LDFLAGS=-L/usr/local/lib

UNAME_S!=uname -s
OUTPUT_BFD=elf64-x86-64
.if ${UNAME_S:MFreeBSD}
OUTPUT_BFD=elf64-x86-64-freebsd
.endif

MB_DIR=data/multiboot
MB_TESTCASES=data_multiboot_mmap.o data_multiboot_modules.o

MOCK_BHYVELOAD=mock/bhyveload.o mock/compat-strlcat.o mock/compat-strlcpy.o

.PHONY: all
all: test-allocator test-loader test-mock-bhyveload test-multiboot

.PHONY: check
check: all
	kyua test
	kyua report

.PHONY: clean
clean:
	$(MAKE) -C data/multiboot clean
	rm -f test-allocator.o
	rm -f test-mock-bhyveload.o
	rm -f test-loader.o
	rm -f test-multiboot.o
	rm -f mock/compat-*.o
	rm -f mock/bhyveload.o
	rm -f test-allocator
	rm -f test-loader
	rm -f test-mock-bhyveload
	rm -f test-multiboot
	rm -f data_*.o
	rm -f *.gcda *.gcno *.gcov
	rm -f .depend.*

.PHONY: ${MB_DIR}/mmap.elf
${MB_DIR}/mmap.elf:
	$(MAKE) -C data/multiboot mmap.elf

.PHONY: ${MB_DIR}/modules.elf
${MB_DIR}/modules.elf:
	$(MAKE) -C data/multiboot modules.elf

data_multiboot_mmap.o: ${MB_DIR}/mmap.elf
	$(OBJCOPY) --input binary --binary-architecture i386 --output ${OUTPUT_BFD} $> $@

data_multiboot_modules.o: ${MB_DIR}/modules.elf
	$(OBJCOPY) --input binary --binary-architecture i386 --output ${OUTPUT_BFD} $> $@

test-multiboot: test-multiboot.o ${MB_TESTCASES} ${MOCK_BHYVELOAD} ../libmultiboot_p.a
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

test-mock-bhyveload: test-mock-bhyveload.o  ${MOCK_BHYVELOAD}
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

test-loader: mock/bhyveload.o ../libmultiboot_p.a

test-allocator: test-allocator.o ../libmultiboot_p.a
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

.c.o:
	$(CC) -c -I.. -Wall ${CFLAGS} -o $@ $>
