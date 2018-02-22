#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <setjmp.h>
#include <string.h>
#include <setjmp.h>

#include <loader.h>
#include <allocator.h>
#include <multiboot.h>
#include <sys/queue.h>

struct loader_callbacks *callbacks;
void *callbacks_arg;

size_t lowmem = 0;
size_t highmem = 0;

jmp_buf jb;

struct args loader_args = {
    .kernel_filename = NULL,
    .cmdline = NULL,
    .modules = SLIST_HEAD_INITIALIZER(.modules),
};

uint32_t
parse_args(struct args* args)
{
    const char *var, *delim, *delim2, *value, *value2, *end;
    int i = 0;
    struct module *new_module;

    /* iterate over environment */
    while ( (var = CALLBACK(getenv, i++)) ) {
        delim = strchr(var, '=');
        value = delim+1;
        end = var + strlen(var);

        if (!delim) {
            /* no value specified */
            delim = var + strlen(var);
            value = NULL;
        }

        printf("%s\r\n", var);
        if (!strncmp(var, "kernel", delim-var)) {
            if (!value) {
                ERROR(EINVAL,"no kernel filename provided");
                return EINVAL;
            }

            args->kernel_filename = value;
        }

        if (!strncmp(var, "cmdline", delim-var)) {
            args->cmdline = value;
        }

        if (!strncmp(var, "module", delim-var)) {
            delim2 = strchr(value, ':');

            /* If no delimiter was found, no optional string was given */
            if (!delim2)
                delim2 = end;

            /* If the delimiter is the last character, remove it. */
            if (delim2 == end - 1) {
                end --;
            }

            /* The optional string starts directly after the deliminator */
            value2 = delim2 + 1;

            new_module = calloc(1, sizeof(struct module));
            new_module->filename = malloc(delim2 - value + 1);
            memcpy(new_module->filename, value, delim2 - value);
            new_module->filename[delim2 - value] = '\0';

            new_module->string = NULL;
            if (delim2 != end) {
                new_module->string = malloc(end - value2 + 1);
                memcpy(new_module->string, value2, end - value2 + 1);
                new_module->string[end - value2] = '\0';
            }

            SLIST_INSERT_HEAD(&loader_args.modules, new_module, next);
        }
    }

    return 0;
}

void
loader_main(struct loader_callbacks *cb, void *arg, int version, int ndisks)
{
    void *kernfile = NULL;
    int mode = 0, uid = 0, gid = 0;

    size_t kernsz = 0, resid = 0;
    void *kernel = NULL;
    struct multiboot *mb = NULL;

    if (version < USERBOOT_VERSION)
        abort();

    callbacks = cb;
    callbacks_arg = arg;

    /* setjmp error anchor */
    if (setjmp(jb)) {
        CALLBACK(exit, errno);
        return;
    }

    if (parse_args(&loader_args))
        goto error;

    if (callbacks->open(callbacks_arg, loader_args.kernel_filename, &kernfile))
    {
        ERROR(errno, "could not open kernel");
        goto error;
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

    /* Scan for the multiboot header */
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
        if (multiboot_load(kernel, kernsz, mb)) {
            goto error;
        }

        if (multiboot_info_set_meminfo(&mb->info, lowmem, highmem)) {
            ERROR(EINVAL, "Could not set up memory information");
            goto error;
        }

        if (multiboot_info_set_mmap(&mb->info)) {
            ERROR(EINVAL, "Could not set up memory map");
            goto error;
        }

        if (loader_args.cmdline &&
            multiboot_info_set_cmdline(&mb->info, loader_args.cmdline))
        {
            ERROR(EINVAL, "Could not set kernel command line");
            goto error;
        }

        if (multiboot_info_set_loader_name(&mb->info, BOOTLOADER_NAME)) {
            ERROR(EINVAL, "Could not set bootloader name");
            goto error;
        }

        if (mb->header.mb.header->flags & MULTIBOOT_FLAG_GRAPHICS) {
            ERROR(ENOTSUP, "VBE info requested by kernel, but not supported.");
        }

        if (multiboot_load_modules(mb, &loader_args.modules)) {
            ERROR(ECANCELED, "Could not load modules");
        }

        if (multiboot_info_finalize(mb)) {
            ERROR(ECANCELED, "Could not set up multiboot information structure");
        }

        if (multiboot_enter(mb)) {
            ERROR(ECANCELED, "Could not set up machine state");
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