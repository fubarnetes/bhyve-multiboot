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

#include <allocator.h>
#include <stdlib.h>
#include <sys/queue.h>

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

typedef struct allocation allocation_t;
typedef TAILQ_HEAD(allocations_head, allocation) allocations_head_t;

struct allocation {
    TAILQ_ENTRY(allocation) entry;
    void* addr;
    size_t size;
};

static allocations_head_t allocations = TAILQ_HEAD_INITIALIZER(allocations);

static void* min_alloc_address = NULL;
static void* max_alloc_address = NULL;

/*
 * Attempt making an allocation, or return if the allocation would go out of
 * usable memory.
 */
#define MK_ALLOCATION(var, at, size) do {\
        if (!(var = _allocation(at, size))) return 0; \
    } while (0)

static allocation_t*
_allocation(void* at, size_t size)
{
    allocation_t *new_allocation = NULL;

    /* Check that this allocation would not go out of usable memory. */
    if ((at < min_alloc_address) || (at + size > max_alloc_address))
        return NULL;

    new_allocation = malloc(sizeof(allocation_t));
    new_allocation->addr = at;
    new_allocation->size = size;

    return new_allocation;
}

void
init_allocator(size_t lowmem, size_t highmem)
{
    /*
     * This function is not reentrant. If max_alloc_address or min_alloc_address
     * are already set, just return.
     */
    if (min_alloc_address || max_alloc_address)
        return;

    /* Start allocating above the 1 MiB watermark, above the VGA hole. */
    min_alloc_address = (void*) (1 * MiB);

    /*
     * Check whether we only have a lowmem segment, or there is also a highmem
     * segment.
     *
     * If there is only a lowmem segment, highmem will be set to 0.
     */
    if (highmem) {
        max_alloc_address = (void*) (4 * GiB) + highmem;

        /*
         * Reserve (allocate) the MMAP hole above the lowmem segment.
         * By default, this starts at 3 GiB (can be changed with
         * vm_set_lowmem_limit, but bhyveload does not do this.)
         */
        allocate_at((void*) lowmem, 4 * GiB - lowmem);
    }
    else {
        max_alloc_address = (void*) lowmem;
    }
}

void*
allocate(size_t size)
{
    allocation_t *it = NULL, *new_allocation = NULL;
    void *last_endp = min_alloc_address; /* pointer to the end of the last allocation seen */

    /*
     * If we do not have any allocations yet, just allocate at the first
     * possible address.
     */
    if (unlikely(TAILQ_EMPTY(&allocations))) {
        MK_ALLOCATION(new_allocation, min_alloc_address, size);
        TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        return (void*) min_alloc_address;
    }

    /* 
     * We never remove anything from the list, so we don't need to use the safe
     * version here.
     */
    TAILQ_FOREACH(it, &allocations, entry) {
        if (last_endp && (it->addr >= last_endp + size)) {
            /* We found a sufficiently large space. */
            MK_ALLOCATION(new_allocation, last_endp, size);
            TAILQ_INSERT_BEFORE(it, new_allocation, entry);
            return last_endp;
        }
        last_endp = it->addr + it->size;
    }

    /*
     * We arrived at the end of the list without finding a sufficiently
     * large free space. Let's allocate one at the end now.
     */
    MK_ALLOCATION(new_allocation, last_endp, size);
    TAILQ_INSERT_TAIL(&allocations, new_allocation, entry);
    return last_endp;
}

static inline void*
__align(void* ptr)
{
    if ((uintptr_t) ptr % PAGESZ == 0)
        return ptr;
    
    return ptr - ((uintptr_t) ptr % PAGESZ) + PAGESZ;
}

void*
allocate_aligned(size_t size)
{
    allocation_t *it = NULL, *new_allocation = NULL;
    void *last_endp = min_alloc_address; /* pointer to the end of the last allocation seen */

    /*
     * If we do not have any allocations yet, just allocate at the first
     * possible address.
     */
    if (unlikely(TAILQ_EMPTY(&allocations))) {
        MK_ALLOCATION(new_allocation, __align(min_alloc_address), size);
        TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        return (void*) min_alloc_address;
    }
    
    /* 
     * We never remove anything from the list, so we don't need to use the safe
     * version here.
     */
    TAILQ_FOREACH(it, &allocations, entry) {
        if (last_endp && (it->addr >= last_endp + size)) {
            /* We found a sufficiently large space. */
            MK_ALLOCATION(new_allocation, last_endp, size);
            TAILQ_INSERT_BEFORE(it, new_allocation, entry);
            return last_endp;
        }
        last_endp = __align(it->addr + it->size);
    }

    /*
     * We arrived at the end of the list without finding a sufficiently
     * large free space. Let's allocate one at the end now.
     */
    MK_ALLOCATION(new_allocation, last_endp, size);
    TAILQ_INSERT_TAIL(&allocations, new_allocation, entry);
    return last_endp;
}

void*
allocate_at(void* at, size_t size)
{
    allocation_t *it = NULL, *new_allocation = NULL;

    /*
     * If we do not have any allocations yet, just allocate the requested chunk.
     */
    if (unlikely(TAILQ_EMPTY(&allocations))) {
        MK_ALLOCATION(new_allocation, at, size);
        TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        return at;
    }

    /* 
     * We never remove anything from the list, so we don't need to use the safe
     * version here.
     */
    TAILQ_FOREACH(it, &allocations, entry) {
        /*
         * Fastpath: skip over any allocation that lies below this allocation.
         */
        if ((it->addr + it->size) <= at)
            continue;
        
        /*
         * If the start address of the requested chunk is within the current
         * chunk, don't allocate anything. Any other chunks cannot be before the
         * requested chunk any more.
         * 
         * Otherwise, the current chunk lies above the requested address.
         */
        if ((it->addr <= at) && (at < (it->addr + it->size)))
            return NULL;
        
        /*
         * Check whether the current chunk's start address would lie in the
         * requested chunk.
         */
        if ((at + size) > it->addr)
            return NULL;

        /* 
         * Otherwise, the request can be accomodated.
         * 
         * If the current chunk is the first chunk, insert the allocation at the
         * list head.
         */
        MK_ALLOCATION(new_allocation, at, size);

        if (it == TAILQ_FIRST(&allocations))
            TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        else
            /* The requested chunk is before the current chunk. */
            TAILQ_INSERT_BEFORE(it, new_allocation, entry);

        return at;
    }

    /* We did not find a chunk that lies after the current chunk. */
    MK_ALLOCATION(new_allocation, at, size);
    TAILQ_INSERT_TAIL(&allocations, new_allocation, entry);
    return at;
}