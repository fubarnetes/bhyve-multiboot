#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <setjmp.h>

#include <loader.h>
#include <allocator.h>
#include <multiboot.h>

struct loader_callbacks *callbacks;
void *callbacks_arg;

size_t lowmem = 0;
size_t highmem = 0;

jmp_buf jb;

void
loader_main(struct loader_callbacks *cb, void *arg, int version, int ndisks)
{
	const char *var, *delim, *value;
    void *kernfile = NULL;
    int mode = 0, uid = 0, gid = 0;

    size_t kernsz = 0, resid = 0;
    void *kernel = NULL;
    struct multiboot *mb;
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

            if (callbacks->open(callbacks_arg, value, &kernfile)) {
                ERROR(errno, "could not open kernel");
                goto error;
            }
		}
    }

    /* Get the memory layout */
    callbacks->getmem(callbacks_arg, &lowmem, &highmem);
    printf("lowmem = %lu, highmem = %lu\r\n", lowmem, highmem);

    /* Initialize the allocator */
    init_allocator(lowmem, highmem);

    /* Check that a kernel file was provided */
    if (!kernfile) {
        ERROR(EINVAL, "No kernel given.");
        goto error;
    }

    /* Get the kernel file size */
    callbacks->stat(callbacks_arg, kernfile, &mode, &uid, &gid, &kernsz);
    printf("kernel size = %ld\r\n", kernsz);

    /* Read the kernel */
    kernel = malloc(kernsz);
    if (!kernel || callbacks->read(callbacks_arg, kernfile,
        kernel, kernsz, &resid))
    {
        ERROR(errno, "Unable to read kernel");
        goto error;
    }

    /* Check that a kernel file was provided */
    if (!(mb = mb_scan(kernel, kernsz))) {
        ERROR(EINVAL, "No multiboot header found.");
        goto error;
    }

    /* We don't support multiboot2 yet */
    if (mb->magic == MULTIBOOT2_MAGIC) {
        ERROR(ENOTSUP, "Multiboot2 is not supported yet.");
        goto error;
    }
    else {
        if (multiboot_load(kernel, kernsz, mb->info.mb.header)) {
            goto error;
        }
    }

    /* Cleanup. */
    if (mb) free(mb);
    if (kernel) free(kernel);
    if (kernfile) callbacks->close(callbacks_arg, kernfile);
	CALLBACK(exit, 0);
	return;

 error:
    if (mb) free(mb);
    if (kernel) free(kernel);
    if (kernfile) callbacks->close(callbacks_arg, kernfile);
	longjmp(jb, 1);
}