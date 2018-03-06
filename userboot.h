/*-
 * Copyright (c) 2011 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/boot/userboot/userboot.h 296099 2016-02-26 16:00:16Z marcel $
 */

#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * USERBOOT interface versions
 */
#define USERBOOT_VERSION_1      1
#define USERBOOT_VERSION_2      2
#define USERBOOT_VERSION_3      3

/**
 * Version 4 added more generic callbacks for setting up
 * registers and descriptors. The callback structure is
 * backward compatible (new callbacks have been added at
 * the tail end).
 */
#define USERBOOT_VERSION_4      4

/*
 * Exit codes from the loader
 */
#define USERBOOT_EXIT_QUIT      1
#define USERBOOT_EXIT_REBOOT    2

enum vm_reg_name {
        VM_REG_GUEST_RAX,
        VM_REG_GUEST_RBX,
        VM_REG_GUEST_RCX,
        VM_REG_GUEST_RDX,
        VM_REG_GUEST_RSI,
        VM_REG_GUEST_RDI,
        VM_REG_GUEST_RBP,
        VM_REG_GUEST_R8,
        VM_REG_GUEST_R9,
        VM_REG_GUEST_R10,
        VM_REG_GUEST_R11,
        VM_REG_GUEST_R12,
        VM_REG_GUEST_R13,
        VM_REG_GUEST_R14,
        VM_REG_GUEST_R15,
        VM_REG_GUEST_CR0,
        VM_REG_GUEST_CR3,
        VM_REG_GUEST_CR4,
        VM_REG_GUEST_DR7,
        VM_REG_GUEST_RSP,
        VM_REG_GUEST_RIP,
        VM_REG_GUEST_RFLAGS,
        VM_REG_GUEST_ES,
        VM_REG_GUEST_CS,
        VM_REG_GUEST_SS,
        VM_REG_GUEST_DS,
        VM_REG_GUEST_FS,
        VM_REG_GUEST_GS,
        VM_REG_GUEST_LDTR,
        VM_REG_GUEST_TR,
        VM_REG_GUEST_IDTR,
        VM_REG_GUEST_GDTR,
        VM_REG_GUEST_EFER,
        VM_REG_GUEST_CR2,
        VM_REG_GUEST_PDPTE0,
        VM_REG_GUEST_PDPTE1,
        VM_REG_GUEST_PDPTE2,
        VM_REG_GUEST_PDPTE3,
        VM_REG_GUEST_INTR_SHADOW,
        VM_REG_LAST
};

struct loader_callbacks {
    /*
     * Console i/o
     */

    /**
     * @brief Wait until a key is pressed on the console and then return it
     */
    int (*getc)(void *arg);

    /**
     * Write the character ch to the console
     */
    void (*putc)(void *arg, int ch);

    /**
     * Return non-zero if a key can be read from the console
     */
    int (*poll)(void *arg);

    /*
     * Host filesystem i/o
     */

    /**
     * Open a file in the host filesystem
     */
    int (*open)(void *arg, const char *filename, void **h_return);

    /**
     * Close a file
     */
    int (*close)(void *arg, void *h);

    /**
     * Return non-zero if the file is a directory
     */
    int (*isdir)(void *arg, void *h);

    /**
     * Read size bytes from a file. The number of bytes remaining
     * in dst after reading is returned in *resid_return
     */
    int (*read)(void *arg, void *h, void *dst, size_t size,
                size_t *resid_return);

    /*
     * Read an entry from a directory. The entry's inode number is
     * returned in *fileno_return, its type in *type_return and
     * the name length in *namelen_return. The name itself is
     * copied to the buffer name which must be at least PATH_MAX
     * in size.
     */
    int (*readdir)(void *arg, void *h, uint32_t *fileno_return,
                   uint8_t *type_return, size_t *namelen_return, char *name);

    /**
     * Seek to a location within an open file
     */
    int (*seek)(void *arg, void *h, uint64_t offset,
                int whence);

    /**
     * Return some stat(2) related information about the file
     */
    int (*stat)(void *arg, void *h, int *mode_return,
                int *uid_return, int *gid_return, uint64_t *size_return);

    /*
     * Disk image i/o
     */

    /**
     * Read from a disk image at the given offset
     */
    int (*diskread)(void *arg, int unit, uint64_t offset,
                    void *dst, size_t size, size_t *resid_return);

    /*
     * Guest virtual machine i/o
     */

    /**
     * Copy to the guest address space
     */
    int (*copyin)(void *arg, const void *from,
                  uint64_t to, size_t size);

    /**
     * Copy from the guest address space
     */
    int (*copyout)(void *arg, uint64_t from,
                   void *to, size_t size);

    /**
     * Set a guest register value
     */
    void (*setreg)(void *arg, int, uint64_t);

    /**
     * Set a guest MSR value
     */
    void (*setmsr)(void *arg, int, uint64_t);

    /**
     * Set a guest CR value
     */
    void (*setcr)(void *arg, int, uint64_t);

    /**
     * Set the guest GDT address
     */
    void (*setgdt)(void *arg, uint64_t, size_t);

    /**
     * Transfer control to the guest at the given address
     */
    void (*exec)(void *arg, uint64_t pc);

    /*
     * Misc
     */

    /**
     * Sleep for usec microseconds
     */
    void (*delay)(void *arg, int usec);

    /**
     * Exit with the given exit code
     */
    void (*exit)(void *arg, int v);

    /**
     * Return guest physical memory map details
     */
    void (*getmem)(void *arg, uint64_t *lowmem,
                   uint64_t *highmem);

    /**
     * ioctl interface to the disk device
     */
    int (*diskioctl)(void *arg, int unit, u_long cmd,
                     void *data);

    /**
     * Returns an environment variable in the form "name=value".
     *
     * If there are no more variables that need to be set in the
     * loader environment then return NULL.
     *
     * 'num' is used as a handle for the callback to identify which
     * environment variable to return next. It will begin at 0 and
     * each invocation will add 1 to the previous value of 'num'.
     */
    const char* (*getenv)(void *arg, int num);

    /*
     * Version 4 additions.
     */
    int    (*vm_set_register)(void *arg, int vcpu, int reg, uint64_t val);
    int    (*vm_set_desc)(void *arg, int vcpu, int reg, uint64_t base,
                          u_int limit, u_int access);

    /*
     * Version 5 additions.
     *
     * vm_get_unrestricted_guest checks support for the UNRESTRICTED_GUEST
     * capability and if supported, sets 'retval'. If unsupported, an error
     * code is returned.
     *
     * vm_set_unrestricted_guest sets the UNRESTRICTED_GUEST capability if
     * supported, and returns an error code otherwise.
     */
    int (*vm_get_unrestricted_guest)(void* arg, int vcpu, int *retval);
    int (*vm_set_unrestricted_guest)(void* arg, int vcpu, int val);
    int (*vcpu_reset)(void* arg, int vcpu);
};
