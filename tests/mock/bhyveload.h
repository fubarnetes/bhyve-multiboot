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

int	vm_set_register(struct vmctx *ctx, int vcpu, int reg, uint64_t val);
int	vm_get_register(struct vmctx *ctx, int vcpu, int reg, uint64_t *retval);
int	vm_set_desc(struct vmctx *ctx, int vcpu, int reg,
		    uint64_t base, uint32_t limit, uint32_t access);
int	vm_get_desc(struct vmctx *ctx, int vcpu, int reg,
		    uint64_t *base, uint32_t *limit, uint32_t *access);
int	vcpu_reset(struct vmctx *ctx, int vcpu);