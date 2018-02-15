LDFLAGS=-L/usr/local/lib
LIBATF_C=/usr/local/lib/libatf-c.a

.PHONY: all
all: test-allocator

.PHONY: check
check: all
	kyua test
	kyua report

.PHONY: clean
clean:
	rm -f test-allocator.o
	rm -f test-allocator

test-allocator: test-allocator.o ../libmultiboot_p.a
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

.c.o:
	$(CC) -c -I.. -Wall ${CFLAGS} -o $@ $>