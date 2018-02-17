LDFLAGS=-L/usr/local/lib
LIBATF_C=/usr/local/lib/libatf-c.a

.PHONY: all
all: test-allocator test-loader test-mock-bhyveload

.PHONY: check
check: all
	kyua test
	kyua report

.PHONY: clean
clean:
	rm -f test-allocator.o
	rm -f test-mock-bhyveload.o
	rm -f mock/bhyveload.o
	rm -f test-allocator
	rm *.gcda *.gcno *.gcov

test-mock-bhyveload: test-mock-bhyveload.o mock/bhyveload.o mock/compat-strlcat.o mock/compat-strlcpy.o
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

test-loader: mock/bhyveload.o ../libmultiboot_p.a

test-allocator: test-allocator.o ../libmultiboot_p.a
	$(CC) ${LDFLAGS} ${CFLAGS} -o $@ $> ${LIBATF_C} ${LIBELF}

.c.o:
	$(CC) -c -I.. -Wall ${CFLAGS} -o $@ $>
