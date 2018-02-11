LIB=		multiboot
SHLIB_NAME=	libmultiboot.so
SRCS=		multiboot.c allocator.c

CFLAGS=		-Wall
CFLAGS+=	-I.
LDADD=		-lelf

LDFLAGS+=	-Wl,-Bsymbolic

SUBDIR+=tests

.include <bsd.lib.mk>
# vim: set noexpandtab ts=4 : 
