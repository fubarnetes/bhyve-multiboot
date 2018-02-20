/*-
 * Copyright (c) 2018 Fabian Freyer <fabian.freyer@physik.tu-berlin.de>
 * Copyright (c) 2011 NetApp, Inc.
 * Copyright (c) 2011 Google, Inc.
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
 */

/*
 * Parts of this file are copied from usr.sbin/bhyveload/bhyveload.c
 */

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <unistd.h>

#include <userboot.h>
#include <tests/mock/bhyveload.h>

#define kiB (size_t) (1024)
#define MiB (size_t) (1024 * 1024)
#define GiB (size_t) (1024 * 1024 * 1024)

#define BSP 0

static char *host_base = ".";
static void cb_exit(void *arg, int v);

struct cb_file {
    int cf_isdir;
    size_t cf_size;
    struct stat cf_stat;
    union {
        int fd;
        DIR *dir;
    } cf_u;
};

static void
cb_putc(void *arg, int ch)
{
    cb_exit(arg, ENOSYS);
}

static int
cb_getc(void *arg)
{
    cb_exit(arg, ENOSYS);
}

static int
cb_poll(void *arg)
{
    cb_exit(arg, ENOSYS);
}

static int
cb_open(void *arg, const char *filename, void **hp)
{
    struct cb_file *cf;
    char path[PATH_MAX];

    if (!host_base)
        return (ENOENT);

    strlcpy(path, host_base, PATH_MAX);
    if (path[strlen(path) - 1] == '/')
        path[strlen(path) - 1] = 0;
    strlcat(path, filename, PATH_MAX);

    cf = malloc(sizeof(struct cb_file));
    if (stat(path, &cf->cf_stat) < 0) {
        free(cf);
        return (errno);
    }

    cf->cf_size = cf->cf_stat.st_size;
    if (S_ISDIR(cf->cf_stat.st_mode)) {
        cf->cf_isdir = 1;
        cf->cf_u.dir = opendir(path);
        if (!cf->cf_u.dir)
            goto out;
        *hp = cf;
        return (0);
    }
    if (S_ISREG(cf->cf_stat.st_mode)) {
        cf->cf_isdir = 0;
        cf->cf_u.fd = open(path, O_RDONLY);
        if (cf->cf_u.fd < 0)
            goto out;
        *hp = cf;
        return (0);
    }

out:
    free(cf);
    return (EINVAL);
}

static int
cb_close(void *arg, void *h)
{
    struct cb_file *cf = h;

    if (cf->cf_isdir)
        closedir(cf->cf_u.dir);
    else
        close(cf->cf_u.fd);
    free(cf);

    return (0);
}

static int
cb_read(void *arg, void *h, void *buf, size_t size, size_t *resid)
{
    struct cb_file *cf = h;
    ssize_t sz;

    if (cf->cf_isdir)
        return (EINVAL);
    sz = read(cf->cf_u.fd, buf, size);
    if (sz < 0)
        return (EINVAL);
    *resid = size - sz;
    return (0);
}

static int
cb_isdir(void *arg, void *h)
{
    struct cb_file *cf = h;

    return (cf->cf_isdir);
}

static int
cb_readdir(void *arg, void *h, uint32_t *fileno_return, uint8_t *type_return,
       size_t *namelen_return, char *name)
{
    struct cb_file *cf = h;
    struct dirent *dp;

    if (!cf->cf_isdir)
        return (EINVAL);

    dp = readdir(cf->cf_u.dir);
    if (!dp)
        return (ENOENT);

    /*
     * Note: strlen(d_name) is in the range 0..255 and therefore less
     * than PATH_MAX so we don't need to test before copying.
     */
    *fileno_return = dp->d_fileno;
    *type_return = dp->d_type;
    *namelen_return = strlen(dp->d_name);
    memcpy(name, dp->d_name, strlen(dp->d_name));
    name[strlen(dp->d_name)] = 0;

    return (0);
}

static int
cb_seek(void *arg, void *h, uint64_t offset, int whence)
{
    struct cb_file *cf = h;

    if (cf->cf_isdir)
        return (EINVAL);
    if (lseek(cf->cf_u.fd, offset, whence) < 0)
        return (errno);
    return (0);
}

static int
cb_stat(void *arg, void *h, int *mode, int *uid, int *gid, uint64_t *size)
{
    struct cb_file *cf = h;

    *mode = cf->cf_stat.st_mode;
    *uid = cf->cf_stat.st_uid;
    *gid = cf->cf_stat.st_gid;
    *size = cf->cf_stat.st_size;
    return (0);
}

static void
cb_exec(void* arg, uint64_t pc)
{
    /*
     * Userboot's exec sets up registers for FreeBSD. This is not compatible
     * with the state defined in the multiboot spec. Don't use cb_exec.
     */
    cb_exit(arg, ENOSYS);
}

static void
cb_delay (void *arg, int usec)
{
    cb_exit(arg, ENOSYS);
}

static int
cb_diskread(void *arg, int unit, uint64_t from, void *to, size_t size,
        size_t *resid)
{
    cb_exit(arg, ENOSYS);
}

static int
cb_diskioctl(void *arg, int unit, u_long cmd, void *data)
{
    cb_exit(arg, ENOSYS);
}

static size_t _lowmem = 0;
static size_t _highmem = 0;
void* highmem_buffer = NULL;
void* lowmem_buffer = NULL;

void
setmem(size_t lowmem, size_t highmem) {
    if (lowmem_buffer) free(lowmem_buffer);
    if (highmem_buffer) free(highmem_buffer);

    _lowmem = lowmem;
    _highmem = highmem;

    lowmem_buffer = calloc(1, lowmem);
    highmem_buffer = calloc(1, highmem);
}

static void
cb_getmem(void *arg, uint64_t *ret_lowmem, uint64_t *ret_highmem)
{

    *ret_lowmem = _lowmem;
    *ret_highmem = _highmem;
}

struct env {
    const char *str;    /* name=value */
    SLIST_ENTRY(env) next;
};

static SLIST_HEAD(envhead, env) envhead;

void
addenv(const char *str)
{
    struct env *env;

    env = malloc(sizeof(struct env));
    env->str = str;
    SLIST_INSERT_HEAD(&envhead, env, next);
}

static const char *
cb_getenv(void *arg, int num)
{
    int i;
    struct env *env;

    i = 0;
    SLIST_FOREACH(env, &envhead, next) {
        if (i == num)
            return (env->str);
        i++;
    }

    return (NULL);
}

int exited = 0;
int exit_reason = 0;

static void
cb_exit(void *arg, int v)
{
    exit_reason = v;
    exited = 1;
}

/*
 * Guest virtual machine i/o callbacks
 */
static int
cb_copyin(void *arg, const void *from, uint64_t to, size_t size)
{
    void *ptr = NULL;
    size_t offset = 0;
    size_t seg_size = 0;

    to &= 0x7fffffff;

    if (to >= 4 * GiB) {
        offset = 4 * GiB;
        ptr = highmem_buffer + to - offset;
        seg_size = _highmem;
    }
    else {
        ptr = lowmem_buffer + to;
        seg_size = _lowmem;
    }

    /* Don't overflow */
    if (to - offset + size > seg_size)
        return EFAULT;

    /* Don't underflow */
    if (to < offset)
        return EFAULT;


    memcpy(ptr, from, size);
    return (0);
}

static int
cb_copyout(void *arg, uint64_t from, void *to, size_t size)
{
    char *ptr;
    size_t offset = 0;
    size_t seg_size = 0;

    from &= 0x7fffffff;

    if (from >= 4 * GiB) {
        offset = 4*GiB;
        ptr = highmem_buffer + from - offset;
        seg_size = _highmem;
    }
    else {
        ptr = lowmem_buffer + from;
        seg_size = _lowmem;
    }

    if (ptr == NULL)
        return (EFAULT);

    /* Don't overflow */
    if (from - offset + size > seg_size)
        return EFAULT;

    /* Don't underflow */
    if (from < offset)
        return EFAULT;

    memcpy(to, ptr, size);
    return (0);
}

struct seg_desc {
    uint64_t        base;
    uint32_t        limit;
    uint32_t        access;
};


/* Mock vm_{get,set}_desc from vmmapi. */
static SLIST_HEAD(vm_seg_desc_head, vm_seg_desc) vm_seg_desc_head;

struct vm_seg_desc { /* data or code segment */
    int     cpuid;
    int regnum;    /* enum vm_reg_name */
    struct seg_desc desc;
    SLIST_ENTRY(vm_seg_desc) next;
};

int
vm_set_desc(struct vmctx *ctx, int vcpu, int reg,
            uint64_t base, uint32_t limit, uint32_t access)
{
    struct vm_seg_desc *segdesc;

    SLIST_FOREACH(segdesc, &vm_seg_desc_head, next) {
        if (segdesc->cpuid != vcpu || segdesc->regnum != reg)
            continue;

        segdesc->desc.base = base;
        segdesc->desc.limit = limit;
        segdesc->desc.access = access;
        return 0;
    }

    segdesc = calloc(1, sizeof(struct vm_seg_desc));
    segdesc->cpuid = vcpu;
    segdesc->regnum = reg;
    segdesc->desc.base = base;
    segdesc->desc.limit = limit;
    segdesc->desc.access = access;
    SLIST_INSERT_HEAD(&vm_seg_desc_head, segdesc, next);
    return 0;
}

int
vm_get_desc(struct vmctx *ctx, int vcpu, int reg,
            uint64_t *base, uint32_t *limit, uint32_t *access)
{
    struct vm_seg_desc *segdesc;

    SLIST_FOREACH(segdesc, &vm_seg_desc_head, next) {
        if (segdesc->cpuid != vcpu || segdesc->regnum != reg)
            continue;

        *base = segdesc->desc.base;
        *limit = segdesc->desc.limit;
        *access = segdesc->desc.access;
        return 0;
    }

    *base = 0;
    *limit = 0;
    *access = 0;
    return 0;
}

/* Mock vm_{get,set}_register from vmmapi. */
static SLIST_HEAD(vm_reg_head, vm_reg) vm_reg_head;

struct vm_reg { /* data or code segment */
    int    cpuid;
    int regnum;    /* enum vm_reg_name */
    uint64_t value;
    SLIST_ENTRY(vm_reg) next;
};

int
vm_set_register(struct vmctx *ctx, int vcpu, int reg, uint64_t val)
{
    struct vm_reg *_reg;
    printf("set register %d = %lu\n", reg, val);

    SLIST_FOREACH(_reg, &vm_reg_head, next) {
        if (_reg->cpuid != vcpu || _reg->regnum != reg)
            continue;

        _reg->value = val;
        return 0;
    }

    _reg = calloc(1, sizeof(struct vm_reg));
    _reg->cpuid = vcpu;
    _reg->regnum = reg;
    _reg->value = val;
    SLIST_INSERT_HEAD(&vm_reg_head, _reg, next);
    return 0;
}

int
vm_get_register(struct vmctx *ctx, int vcpu, int reg, uint64_t *retval)
{
    struct vm_reg *_reg;

    SLIST_FOREACH(_reg, &vm_reg_head, next) {
        if (_reg->cpuid != vcpu || _reg->regnum != reg)
            continue;

        *retval = _reg->value;
        printf("get register %d = %lu\n", reg, *retval);
        return 0;
    }

    return 0;
}

/*
 * The following function is taken from vmmapi.c
 * From Intel Vol 3a:
 * Table 9-1. IA-32 Processor States Following Power-up293gg, Reset or INIT
 */
#define      CR0_NE  0x00000020
int
vcpu_reset(struct vmctx *vmctx, int vcpu)
{
    int error;
    uint64_t rflags, rip, cr0, cr4, zero, desc_base, rdx;
    uint32_t desc_access, desc_limit;
    uint16_t sel;

    zero = 0;

    rflags = 0x2;
    error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RFLAGS, rflags);
    if (error)
        goto done;

    rip = 0xfff0;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RIP, rip)) != 0)
        goto done;

    cr0 = CR0_NE;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_CR0, cr0)) != 0)
        goto done;

    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_CR3, zero)) != 0)
        goto done;
    
    cr4 = 0;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_CR4, cr4)) != 0)
        goto done;

    /*
     * CS: present, r/w, accessed, 16-bit, byte granularity, usable
     */
    desc_base = 0xffff0000;
    desc_limit = 0xffff;
    desc_access = 0x0093;
    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_CS,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    sel = 0xf000;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_CS, sel)) != 0)
        goto done;

    /*
     * SS,DS,ES,FS,GS: present, r/w, accessed, 16-bit, byte granularity
     */
    desc_base = 0;
    desc_limit = 0xffff;
    desc_access = 0x0093;
    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_SS,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_DS,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_ES,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_FS,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_GS,
                desc_base, desc_limit, desc_access);
    if (error)
        goto done;

    sel = 0;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_SS, sel)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_DS, sel)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_ES, sel)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_FS, sel)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_GS, sel)) != 0)
        goto done;

    /* General purpose registers */
    rdx = 0xf00;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RAX, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RBX, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RCX, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RDX, rdx)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RSI, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RDI, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RBP, zero)) != 0)
        goto done;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_RSP, zero)) != 0)
        goto done;

    /* GDTR, IDTR */
    desc_base = 0;
    desc_limit = 0xffff;
    desc_access = 0;
    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_GDTR,
                desc_base, desc_limit, desc_access);
    if (error != 0)
        goto done;

    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_IDTR,
                desc_base, desc_limit, desc_access);
    if (error != 0)
        goto done;

    /* TR */
    desc_base = 0;
    desc_limit = 0xffff;
    desc_access = 0x0000008b;
    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_TR, 0, 0, desc_access);
    if (error)
        goto done;

    sel = 0;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_TR, sel)) != 0)
        goto done;

    /* LDTR */
    desc_base = 0;
    desc_limit = 0xffff;
    desc_access = 0x00000082;
    error = vm_set_desc(vmctx, vcpu, VM_REG_GUEST_LDTR, desc_base,
                desc_limit, desc_access);
    if (error)
        goto done;

    sel = 0;
    if ((error = vm_set_register(vmctx, vcpu, VM_REG_GUEST_LDTR, 0)) != 0)
        goto done;

    /* XXX cr2, debug registers */

    error = 0;
done:
    return (error);
}

static int
cb_vm_set_register(void *arg, int vcpu, int reg, uint64_t val)
{
    struct vmctx ctx;
    return (vm_set_register(&ctx, vcpu, reg, val));
}

static int
cb_vm_set_desc(void *arg, int vcpu, int reg, uint64_t base, u_int limit,
    u_int access)
{
    struct vmctx ctx;
    return (vm_set_desc(&ctx, vcpu, reg, base, limit, access));
}

static void
cb_setreg(void *arg, int r, uint64_t v)
{
    int error;
    enum vm_reg_name vmreg;
    struct vmctx ctx;

    vmreg = VM_REG_LAST;

    switch (r) {
    case 4:
        vmreg = VM_REG_GUEST_RSP;
        break;
    default:
        break;
    }

    if (vmreg == VM_REG_LAST) {
        printf("test_setreg(%d): not implemented\n", r);
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }

    error = vm_set_register(&ctx, BSP, vmreg, v);
    if (error) {
        perror("vm_set_register");
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }
}

static void
cb_setmsr(void *arg, int r, uint64_t v)
{
    int error;
    enum vm_reg_name vmreg;
    struct vmctx ctx;

    vmreg = VM_REG_LAST;

    switch (r) {
    case MSR_EFER:
        vmreg = VM_REG_GUEST_EFER;
        break;
    default:
        break;
    }

    if (vmreg == VM_REG_LAST) {
        printf("test_setmsr(%d): not implemented\n", r);
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }

    error = vm_set_register(&ctx, BSP, vmreg, v);
    if (error) {
        perror("vm_set_msr");
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }
}

static void
cb_setcr(void *arg, int r, uint64_t v)
{
    struct vmctx ctx;
    int error;
    enum vm_reg_name vmreg;

    vmreg = VM_REG_LAST;

    switch (r) {
    case 0:
        vmreg = VM_REG_GUEST_CR0;
        break;
    case 3:
        vmreg = VM_REG_GUEST_CR3;
        break;
    case 4:
        vmreg = VM_REG_GUEST_CR4;
        break;
    default:
        break;
    }

    if (vmreg == VM_REG_LAST) {
        printf("test_setcr(%d): not implemented\n", r);
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }

    error = vm_set_register(&ctx, BSP, vmreg, v);
    if (error) {
        perror("vm_set_cr");
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }
}

static void
cb_setgdt(void *arg, uint64_t base, size_t size)
{
    struct vmctx ctx;
    int error;

    error = vm_set_desc(&ctx, BSP, VM_REG_GUEST_GDTR, base, size - 1, 0);
    if (error != 0) {
        perror("vm_set_desc(gdt)");
        cb_exit(NULL, USERBOOT_EXIT_QUIT);
    }
}

struct loader_callbacks mock_callbacks = {
    .getc = cb_getc,
    .putc = cb_putc,
    .poll = cb_poll,
    
    .open = cb_open,
    .close = cb_close,
    .isdir = cb_isdir,
    .read = cb_read,
    .readdir = cb_readdir,
    .seek = cb_seek,
    .stat = cb_stat,

    .diskread = cb_diskread,
    .diskioctl = cb_diskioctl,

    .copyin = cb_copyin,
    .copyout = cb_copyout,

    .setreg = cb_setreg,
    .setmsr = cb_setmsr,
    .setcr = cb_setcr,
    .setgdt = cb_setgdt,

    .exec = cb_exec,
    .delay = cb_delay,
    .exit = cb_exit,
    .getmem = cb_getmem,

    .getenv = cb_getenv,

    .vm_set_register = cb_vm_set_register,
    .vm_set_desc = cb_vm_set_desc,
};
