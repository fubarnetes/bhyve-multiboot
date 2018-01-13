#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <setjmp.h>

#include <libmultiboot.h>

#define USERBOOT_VERSION 4
#define MALLOCSZ	(64*1024*1024)

#define MULTIBOOT_MAGIC 0x1BADB002

#define ERROR(err, str) do { \
			errno = err; \
			perror(str); \
		} while(0)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

struct loader_callbacks *callbacks;
void *callbacks_arg;

static jmp_buf jb;

struct multiboot_header {
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
};

/*
* Find the location of the multiboot header. 
* The Multiboot header must be contained completely within the first 8192
* bytes of the OS image, and must be longword (32-bit) aligned.
*/
struct multiboot_header* mb_scan(void *kernel, size_t kernsz) {
    uint32_t* magic = NULL;
    printf("Scanning for multiboot header...\n");
    for (magic = kernel; (void*) magic < (kernel + MIN(kernsz, 8192) - sizeof(struct multiboot_header)); magic++) {
        if (*magic != MULTIBOOT_MAGIC)
            continue;

        return (struct multiboot_header *) magic;
    }

    return NULL;
}

void
loader_main(struct loader_callbacks *cb, void *arg, int version, int ndisks)
{
	const char *var, *delim, *value;
	FILE *kernfile = NULL;
    size_t kernsz = 0;
    void *kernel = NULL;
    struct multiboot_header *mb;
	int i = 0;

	if (version < USERBOOT_VERSION)
		abort();

	callbacks = cb;
	callbacks_arg = arg;

	/* setjmp error anchor */
	if (setjmp(jb))
		return;

	/* iterate over environment */
	while ( (var = CALLBACK(getenv, i++)) ) {
		delim = strchr(var, '=');
		value = delim+1;

		if (!delim) {
			/* no value specified */
			delim = var + strlen(var);
			value = NULL;
		}

		printf("%s\r\n", var);
		if (!strncmp(var, "kernel", delim-var)) {
			if (!value) {
				ERROR(EINVAL,"no kernel filename provided");
				goto error;
			}

			kernfile = fopen(value, "r");
            if (!kernfile) {
                ERROR(errno, "could not open kernel");
                goto error;
            }
		}
    }
	
    /* Check that a kernel file was provided */
    if (!kernfile) {
        ERROR(EINVAL, "No kernel given.");
        goto error;
    }

    /* Map the kernel */
    fseek(kernfile, 0L, SEEK_END);
    kernsz = ftell(kernfile);
    rewind(kernfile);
    printf("kernel size = %ld\n", kernsz);

    kernel = mmap(NULL, kernsz, PROT_READ, MAP_PRIVATE, fileno(kernfile), 0);
    if (kernel == MAP_FAILED) {
        ERROR(errno, "Unable to map kernel");
        goto error;
    }

    /* Check that a kernel file was provided */
    if (!(mb = mb_scan(kernel, kernsz))) {
        ERROR(EINVAL, "No multiboot header found.");
        goto error;
    }
    printf("multiboot header at offset %lx\n", ((void*)mb - kernel));

 cleanup:
    if (kernel != MAP_FAILED) munmap(kernel, kernsz);
	fclose(kernfile);
	CALLBACK(exit, 0);
	return;

 error:
    if (kernel != MAP_FAILED) munmap(kernel, kernsz);
	if (kernfile) fclose(kernfile);
	longjmp(jb, 1);
}

/* vim: set noexpandtab ts=4 : */ 
