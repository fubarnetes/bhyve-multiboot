CFLAGS+=	-Wall
CFLAGS+=    -g

OBJS=       loader.o multiboot.o allocator.o

LIBELF=     -lelf
.export LIBELF

.PHONY: all
all: libmultiboot.so libmultiboot.so.full libmultiboot.a libmultiboot_p.a

.PHONY: clean
clean:
	rm -f libmultiboot.a
	rm -f libmultiboot_p.a
	rm -f libmultiboot.so
	rm -f libmultiboot.so.full
	rm -f ${OBJS}
	rm -f *.gcda *.gcno *.gcov
	rm -f .depend.*
	$(MAKE) -C tests clean

.PHONY: check
check:
	$(MAKE) -C tests check

libmultiboot.a: ${OBJS}
	ar rcs $@ $>
	strip $@

libmultiboot_p.a: ${OBJS}
	ar rcs $@ $>

libmultiboot.so: ${OBJS}
	$(CC) -shared ${CFLAGS} -Wl,-Bsymbolic ${LDFLAGS} -o $@ $> ${LDADD} ${LIBELF}
	strip $@

libmultiboot.so.full: ${OBJS}
	$(CC) -shared ${CFLAGS} -Wl,-Bsymbolic ${LDFLAGS} -o $@ $> ${LDADD} ${LIBELF}

.c.o:
	$(CC) -c -fPIC -I. -Wall ${CFLAGS} -o $@ $>
