#pragma once
#include <sys/types.h>
#include <userboot.h>

extern int exited;
extern int exit_reason;

extern void* highmem_buffer;
extern void* lowmem_buffer;

extern struct loader_callbacks mock_callbacks;

void addenv(const char *str);
void setmem(size_t lowmem, size_t highmem);


/* vmmapi foo */
#define MSR_EFER 0xc0000080

/* opaque struct */
struct vmctx {};

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

int	vm_set_register(struct vmctx *ctx, int vcpu, int reg, uint64_t val);
int	vm_get_register(struct vmctx *ctx, int vcpu, int reg, uint64_t *retval);
int	vm_set_desc(struct vmctx *ctx, int vcpu, int reg,
		    uint64_t base, uint32_t limit, uint32_t access);
int	vm_get_desc(struct vmctx *ctx, int vcpu, int reg,
		    uint64_t *base, uint32_t *limit, uint32_t *access);
int	vcpu_reset(struct vmctx *ctx, int vcpu);