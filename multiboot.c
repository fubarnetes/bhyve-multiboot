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
#include <string.h>
#include <errno.h>

#include <sys/types.h>

#include <loader.h>
#include <multiboot.h>

#include <libelf.h>
#include <gelf.h>

#include <allocator.h>

void *entry = NULL;

uint32_t
multiboot_info_set_meminfo(struct multiboot_info* info,
                           uint32_t mem_lower, uint32_t mem_upper)
{
    info->mem_lower = mem_lower;
    info->mem_upper = mem_upper;
    info->flags |= MULTIBOOT_MEMINFO;
    return 0;
}

uint32_t
multiboot_info_set_mmap(struct multiboot_info* info)
{
    uint32_t nsegs = 2;
    uint32_t error = 0;

    struct multiboot_mmap_entry *mmap;
    mmap = calloc(3, sizeof(struct multiboot_mmap_entry));
    mmap[0].size = sizeof(struct multiboot_mmap_entry);
    mmap[0].base_addr = 0;
    mmap[0].length = 640 * kiB;
    mmap[0].type = MULTIBOOT_MMAP_AVAILABLE;
    mmap[1].size = sizeof(struct multiboot_mmap_entry);
    mmap[1].base_addr = 1 * MiB;
    mmap[1].length = info->mem_lower - 1*MiB;
    mmap[1].type = MULTIBOOT_MMAP_AVAILABLE;

    if(info->mem_upper) {
        nsegs = 3;
        mmap[2].size = sizeof(struct multiboot_mmap_entry);
        mmap[2].base_addr = 4 * GiB;
        mmap[2].length = info->mem_upper;
        mmap[2].type = MULTIBOOT_MMAP_AVAILABLE;
    }

    /*
     * There is a discrepancy with the spec here. While the multiboot spec
     * version 0.6.96 specifies a negative offset for the size entry, grub as
     * well as most other bootloaders have the size at offset 0.
     * Therefore we just point mmap_addr directly at a struct starting with
     * size as the first element.
     */
    info->mmap_length = nsegs * sizeof(struct multiboot_mmap_entry);
    info->mmap_addr = (uint32_t) allocate(info->mmap_length);

    if (!info->mmap_addr) {
        error = ENOMEM;
        goto out;
    }

    error = CALLBACK(copyin, mmap, info->mmap_addr, info->mmap_length);
    info->flags |= MULTIBOOT_MMAP;

 out:
    free(mmap);
    return error;
}

uint32_t
multiboot_info_set_loader_name(struct multiboot_info* info, const char* name)
{
    uint32_t error = 0;
    size_t length = strlen(name);
    void* p_addr = allocate(length+1);

    if (!p_addr)
        return ENOMEM;

    error = CALLBACK(copyin, name, p_addr, length+1);
    info->boot_loader_name = p_addr;
    info->flags |= MULTIBOOT_BOOTLOADER_NAME;
    return error;
}

uint32_t
multiboot_info_set_cmdline(struct multiboot_info* info, const char* cmdline)
{
    uint32_t error = 0;
    size_t length = strlen(cmdline);
    void* p_addr = allocate(length+1);

    if (!p_addr)
        return ENOMEM;

    error = CALLBACK(copyin, cmdline, p_addr, length+1);
    info->cmdline = p_addr;
    info->flags |= MULTIBOOT_CMDLINE;
    return error;
}

uint32_t
multiboot_info_finalize(struct multiboot *mb)
{
    uint32_t error = 0;

    void* p_addr = allocate(sizeof(struct multiboot_info));
    if (!p_addr)
        return ENOMEM;

    error = CALLBACK(copyin, &mb->info, p_addr, sizeof(struct multiboot_info));
    if (error)
        return error;

    /*
     * Multiboot specification, section 3.2:
     * EBX Must contain the 32-bit physical address of the Multiboot information
     * structure provided by the boot loader (see spec, section 3.3).
     */
    error = CALLBACK(vm_set_register, 0, VM_REG_GUEST_RBX, p_addr);

    return error;
}

struct multiboot*
mb_scan(void *kernel, size_t kernsz)
{
    struct multiboot *mb = calloc(1, sizeof(struct multiboot));
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
            mb->header.mb.header = (struct multiboot_header*) magic;

            /* Check whether the Multiboot header is contained completely
             * within the first 8192 bytes required by the spec. */
            if ((void*) magic >= kernel + 8192
                - sizeof(struct multiboot_header))
            {
                ERROR(EINVAL,
                    "Multiboot header found, but not in the first 8192 bytes.");
                continue;
            }

            if ((uint32_t) (mb->header.mb.header->magic
                + mb->header.mb.header->flags + mb->header.mb.header->checksum))
            {
                ERROR(EINVAL,
                    "Multiboot header has invalid checksum.");
                continue;
            }

            printf("Multiboot magic at offset 0x%lx\r\n",
                   ((void*)magic - kernel));
        }
        else if (mb->magic == MULTIBOOT2_MAGIC) {
            mb->header.mb2.header = (struct multiboot2_header*) magic;
            if ((void*) magic >= kernel + 32768
                - mb->header.mb2.header->header_length)
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
                     struct multiboot *mb)
{
    struct multiboot_header *mbh = mb->header.mb.header;

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
    if (mbh->flags & MULTIBOOT_AOUT_KLUDGE)
        return LOAD_AOUT;

    return LOAD_ELF;
}

uint32_t
multiboot_load_aout(void* kernel, size_t kernsz, struct multiboot *mb)
{
    struct multiboot_header *mbh = mb->header.mb.header;
    size_t offset = 0;
    size_t loadsz = 0;
    size_t bsssz = 0;

    if (!(mbh->flags & MULTIBOOT_AOUT_KLUDGE)) {
        ERROR(ENOEXEC, "Kernel does not support a.out kludge");
        return ENOEXEC;
    }

    /* Implement a.out loader */
    printf("Using load addresses specified in Multiboot header:\r\n");
    printf("  load @ 0x%08x - 0x%08x\r\n", mbh->load_addr,
            mbh->load_end_addr);
    printf("  bss @ 0x%08x - 0x%08x\r\n", mbh->load_end_addr,
            mbh->bss_end_addr);
    printf("  entry @ 0x%08x\r\n", mbh->entry_addr);

    /* load_addr must be less than or equal to header_addr. */
    if (mbh->load_addr > mbh->header_addr) {
        ERROR(EINVAL,
                "Loader address must be less than or equal to header_addr.");
        return EINVAL;
    }

    /*
     * The offset in the OS image file at which to start loading is defined by
     * the offset at which the header was found, minus (header_addr - load_addr)
     */
    offset = ((void*) mbh - kernel) - (mbh->header_addr - mbh->load_addr);

    /*
     * If load_end_addr is zero, assume that the text and data segments occupy
     * the whole OS image file.
     */
    if (!mbh->load_end_addr) {
        loadsz = (kernsz - offset);
    }
    else {
        /* (load_end_addr - load_addr) specifies how much data to load. */
        loadsz = mbh->load_end_addr - mbh->load_addr;
    }

    if (!allocate_at((void*) (uintptr_t) mbh->load_addr, loadsz)) {
        /*
         * Could not allocate memory for the text and data, something else is
         * there already!
         */
        ERROR(ENOMEM, "could not allocate segment for text and data section");
        return ENOMEM;
    }

    CALLBACK(copyin,
            kernel+offset,
            mbh->load_addr,
            loadsz);

    if (mbh->bss_end_addr) {
        /*
         * Handle .bss sections.
         */
        if (mbh->bss_end_addr < mbh->load_end_addr) {
            ERROR(EINVAL, "bss_end_addr < mb->load_end_addr");
            return EINVAL;
        }

        bsssz = mbh->bss_end_addr - mbh->load_end_addr;

        if (!allocate_at((void*) (uintptr_t) mbh->load_end_addr, bsssz)) {
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

    entry = (void*) (uintptr_t) mbh->entry_addr;

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
multiboot_load(void* kernel, size_t kernsz, struct multiboot *mb)
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
