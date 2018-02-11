#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <setjmp.h>
#include <sys/queue.h>

#include <libmultiboot.h>
#include <multiboot.h>

#include <libelf.h>
#include <gelf.h>

struct loader_callbacks *callbacks;
void *callbacks_arg;

static jmp_buf jb;

/**
 * @brief find the location of the Multiboot or Multiboot2 header.
 *
 * The Multiboot header must be contained completely within the first 8192
 * bytes of the OS image, and must be longword (32-bit) aligned.
 * The Multiboot2 header must be contained completely within the first 32768
 * bytes of the OS image, and must be 64-bit aligned.
 */
struct multiboot*
mb_scan(void *kernel, size_t kernsz)
{
    struct multiboot *mb = malloc(sizeof(struct multiboot));
    uint32_t* magic = NULL;

    printf("Scanning for multiboot header...\r\n");

    for (magic = kernel;
        (void*) magic < (
            kernel + MIN(kernsz, 32768)
            - MAX(sizeof(struct multiboot2_header),
                  sizeof(struct multiboot_header)));
        magic++)
    {
        if ((*magic != MULTIBOOT1_MAGIC) && (*magic != MULTIBOOT2_MAGIC))
            continue;

        mb->magic = *magic;

        /* For now, let's prefer Multiboot over Multiboot2,  as we don't aim to
         * support that yet. */
        if (mb->magic == MULTIBOOT1_MAGIC) {
            mb->info.mb.header = (struct multiboot_header*) magic;

            /* Check whether the Multiboot header is contained completely
             * within the first 8192 bytes required by the spec. */
            if ((void*) magic >= kernel + 8192
                - sizeof(struct multiboot_header))
            {
                ERROR(EINVAL,
                    "Multiboot header found, but not in the first 8192 bytes.");
                continue;
            }

            if ((uint32_t) (mb->info.mb.header->magic
                + mb->info.mb.header->flags + mb->info.mb.header->checksum))
            {
                ERROR(EINVAL,
                    "Multiboot header has invalid checksum.");
                continue;
            }

            printf("Multiboot magic at offset 0x%lx\r\n",
                   ((void*)magic - kernel));
        }
        else if (mb->magic == MULTIBOOT2_MAGIC) {
            mb->info.mb2.header = (struct multiboot2_header*) magic;
            if ((void*) magic >= kernel + 8192
                - mb->info.mb2.header->header_length)
            {
                ERROR(EINVAL,
                    "Multiboot2 header found, but not in the first 32 kiB.");
                continue;
            }

            printf("Multiboot2 magic at offset 0x%lx\r\n",
                   ((void*)magic - kernel));
        }

        return mb;
    }

    return NULL;
}

uint32_t
multiboot_load(void* kernel, size_t kernsz, struct multiboot_header *mb)
{
    Elf *kernel_elf = NULL;

    /* Check if the file is an ELF */
    if (elf_version(EV_CURRENT) == EV_NONE) {
        ERROR(ENOTSUP, "Wrong libelf version.");
        return ENOTSUP;
    }

    if (((kernel_elf = elf_memory(kernel, kernsz)) == NULL)
        || (elf_kind(kernel_elf) != ELF_K_ELF))
    {
        /* Not an ELF */
        if (!(mb->flags & MULTIBOOT_AOUT_KLUDGE)) {
            ERROR(ENOEXEC, "Kernel is not an ELF or has address information in"
                "Multiboot header.");
            return ENOEXEC;
        }
    }
    else  if (mb->flags & MULTIBOOT_AOUT_KLUDGE) {
        printf("Using load addresses specified in Multiboot header:\r\n");
        printf("  load @ 0x%08x - 0x%08x\r\n", mb->load_addr,
                mb->load_end_addr);
        printf("  entry @ 0x%08x\r\n", mb->entry_addr);
    }
    else {
        /* FIXME: Implement an ELF loader. */
    }

    return 0;
}

void
loader_main(struct loader_callbacks *cb, void *arg, int version, int ndisks)
{
	const char *var, *delim, *value;
	FILE *kernfile = NULL;
    size_t kernsz = 0;
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

    /* Get the kernel file size */
    fseek(kernfile, 0L, SEEK_END);
    kernsz = ftell(kernfile);
    rewind(kernfile);
    printf("kernel size = %ld\r\n", kernsz);

    /* Map the kernel */
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
    if (kernel != MAP_FAILED) munmap(kernel, kernsz);
	fclose(kernfile);
	CALLBACK(exit, 0);
	return;

 error:
    if (mb) free(mb);
    if (kernel != MAP_FAILED) munmap(kernel, kernsz);
	if (kernfile) fclose(kernfile);
	longjmp(jb, 1);
}

/* vim: set noexpandtab ts=4 : */ 