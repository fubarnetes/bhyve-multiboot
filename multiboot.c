/*
 * Copyright (c) 2018, Fabian Freyer <fabian.freyer@physik.tu-berlin.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>

#include <loader.h>
#include <multiboot.h>

#include <libelf.h>
#include <gelf.h>

#include <allocator.h>

void *entry = NULL;

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
            if ((void*) magic >= kernel + 32768
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

    free(mb);
    return NULL;
}

enum LOAD_TYPE
multiboot_load_type (void* kernel, size_t kernsz, Elf **kernel_elf,
                     struct multiboot_header *mb)
{
    /* Initialize libelf */
    if (elf_version(EV_CURRENT) == EV_NONE) {
        ERROR(ENOTSUP, "Wrong libelf version.");
        return ENOTSUP;
    }

    /* Check if the file is an ELF */
    if (((*kernel_elf = elf_memory(kernel, kernsz)) == NULL)
        || (elf_kind(*kernel_elf) != ELF_K_ELF))
    {
        /* Not an ELF. Try loading it as an a.out. */
        elf_end(*kernel_elf);
        *kernel_elf = NULL;
        return LOAD_AOUT;
    }

    /*
     * This kernel is an ELF, but if it has address information set in the
     * multiboot header, try loading it as an a.out.
     */
    if (mb->flags & MULTIBOOT_AOUT_KLUDGE)
        return LOAD_AOUT;

    return LOAD_ELF;
}

uint32_t
multiboot_load_aout(void* kernel, size_t kernsz, struct multiboot_header *mb)
{
    size_t offset = 0;
    size_t loadsz = 0;
    size_t bsssz = 0;
    if (!(mb->flags & MULTIBOOT_AOUT_KLUDGE)) {
        ERROR(ENOEXEC, "Kernel does not support a.out kludge");
        return ENOEXEC;
    }

    /* Implement a.out loader */
    printf("Using load addresses specified in Multiboot header:\r\n");
    printf("  load @ 0x%08x - 0x%08x\r\n", mb->load_addr,
            mb->load_end_addr);
    printf("  bss @ 0x%08x - 0x%08x\r\n", mb->load_end_addr,
            mb->bss_end_addr);
    printf("  entry @ 0x%08x\r\n", mb->entry_addr);

    /* load_addr must be less than or equal to header_addr. */
    if (mb->load_addr > mb->header_addr) {
        ERROR(EINVAL,
                "Loader address must be less than or equal to header_addr.");
        return EINVAL;
    }

    /*
     * The offset in the OS image file at which to start loading is defined by
     * the offset at which the header was found, minus (header_addr - load_addr)
     */
    offset = ((void*) mb - kernel) - (mb->header_addr - mb->load_addr);

    /*
     * If load_end_addr is zero, assume that the text and data segments occupy
     * the whole OS image file.
     */
    if (!mb->load_end_addr) {
        loadsz = (kernsz - offset);
    }
    else {
        /* (load_end_addr - load_addr) specifies how much data to load. */
        loadsz = mb->load_end_addr - mb->load_addr;
    }

    if (!allocate_at((void*) (uintptr_t) mb->load_addr, loadsz)) {
        /*
         * Could not allocate memory for the text and data, something else is
         * there already!
         */
        ERROR(ENOMEM, "could not allocate segment for text and data section");
        return ENOMEM;
    }

    CALLBACK(copyin,
            kernel+offset,
            mb->load_addr,
            loadsz);

    if (mb->bss_end_addr) {
        /*
         * Handle .bss sections.
         */
        if (mb->bss_end_addr < mb->load_end_addr) {
            ERROR(EINVAL, "bss_end_addr < mb->load_end_addr");
            return EINVAL;
        }

        bsssz = mb->bss_end_addr - mb->load_end_addr;

        if (!allocate_at((void*) (uintptr_t) mb->load_end_addr, bsssz)) {
            /*
             * Could not allocate memory for the bss, something else is
             * there already!
             */
            ERROR(ENOMEM, "could not allocate memory for bss section");
            return ENOMEM;
        }
        /*
         * TODO: guarantee that that the bss section is initialized to 0.
         *
         * Check whether that might already be the case for bhyve.
         *
         * Since libuserboot doesn't really have memset(3) callback, we probably
         * have to allocate a whole page using calloc(3) and copy that in, and
         * free it again.
         */
    }

    entry = (void*) (uintptr_t) mb->entry_addr;

    return 0;
}

uint32_t
multiboot_load_elf(void *kernel, size_t kernsz, Elf *kernel_elf) {
    size_t elf_phnum = 0;
    int elf_phidx = 0;
    GElf_Ehdr elf_ehdr;
    GElf_Phdr elf_phdr;

    /* Get the elf Ehdr */
    if (!gelf_getehdr(kernel_elf, &elf_ehdr)) {
        ERROR(EINVAL, "Could not get number of ELF headers");
        return EINVAL;
    }

    /* Get the number of ELF program headers. */
    if (!elf_getphnum(kernel_elf, &elf_phnum)) {
        ERROR(EINVAL, "Could not get number of ELF program headers.");
        return EINVAL;
    }

    if (elf_ehdr.e_machine != EM_X86_64 &&
        elf_ehdr.e_machine != EM_386 &&
        elf_ehdr.e_machine != EM_486)
    {
        ERROR(ENOEXEC, "Wrong architecture");
        return ENOEXEC;
    }

    if (elf_ehdr.e_type != ET_EXEC && elf_ehdr.e_type != ET_DYN) {
        ERROR(ENOEXEC, "ELF is not executable or a dynamic object");
        return ENOEXEC;
    }

    for (elf_phidx = 0; elf_phidx < elf_phnum; elf_phidx++) {
        gelf_getphdr(kernel_elf, elf_phidx, &elf_phdr);

        if (elf_phdr.p_type != PT_LOAD)
            continue;

        printf("loadable segment @%p, offset %lu, len=%lu\n",
            (void*) elf_phdr.p_paddr,
            elf_phdr.p_offset,
            elf_phdr.p_filesz);

        /*
         * For PT_LOAD segments, p_memsz should be smaller than, or equal to
         * p_filesz. Make sure this is actually the case, so we don't write
         * out-of-bounds.
         */
        if (elf_phdr.p_filesz > elf_phdr.p_memsz) {
            ERROR(EINVAL, "p_filesz is larger than p_memsz");
            return EINVAL;
        }

        /*
         * Make sure we don't read outside the kernel elf.
         * FIXME: p_offset + p_filesz can overflow here. Check that.
         */
        if (elf_phdr.p_offset + elf_phdr.p_filesz > kernsz) {
            ERROR(EINVAL, "p_offset larger than elf size.");
            return EINVAL;
        }

        if (!allocate_at((void*) elf_phdr.p_paddr, elf_phdr.p_memsz)) {
            /*
             * Could not allocate memory for the segment, something else is
             * there already!
             */
            ERROR(ENOMEM, "could not allocate segment for segment");
            return ENOMEM;
        }

        callbacks->copyin(callbacks_arg,
            kernel+elf_phdr.p_offset,
            elf_phdr.p_paddr,
            elf_phdr.p_filesz);

        /*
         * TODO: Handle segment .bss sections.
         *
         * If this segment has a .bss section, guarantee that that it is
         * initialized to 0.
         *
         * Check whether that might already be the case for bhyve.
         *
         * Since libuserboot doesn't really have memset(3) callback, we probably
         * have to allocate a whole page using calloc(3) and copy that in, and
         * free it again.
         */
    }

    entry = (void*) elf_ehdr.e_entry;
    return 0;
}

uint32_t
multiboot_load(void* kernel, size_t kernsz, struct multiboot_header *mb)
{
    Elf *kernel_elf = NULL;
    uint32_t error = 0;

    switch (multiboot_load_type(kernel, kernsz, &kernel_elf, mb)) {
        case LOAD_AOUT:
            error = multiboot_load_aout(kernel, kernsz, mb);
            return error;

        case LOAD_ELF:
            error = multiboot_load_elf(kernel, kernsz, kernel_elf);
            if (kernel_elf)
                elf_end(kernel_elf);
            return error;
    }
}

/* vim: set noexpandtab ts=4 : */ 
