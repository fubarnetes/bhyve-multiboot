.include <bsd.init.mk>

LDFLAGS=-L/usr/local/lib
LIBATF_C=/usr/local/lib/libatf-c.a
LDADD_atf_c=-latf-c

CFLAGS+=		-Wall
CFLAGS+=	    -I..

ATF_TESTS_C+=test-allocator
LDADD.test-allocator+=../libmultiboot_p.a
DPADD.test-allocator+=../libmultiboot_p.a

.include <bsd.test.mk>