SHLIB_NAME=	multiboot.so
SRCS=		multiboot.c

CFLAGS=		-Wall
CFLAGS+=	-I.
LDADD=		-lelf

LDFLAGS+=	-Wl,-Bsymbolic

.include <bsd.lib.mk>
# vim: set noexpandtab ts=4 : 
