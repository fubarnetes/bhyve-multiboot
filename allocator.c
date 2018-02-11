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

static allocation_t*
_allocation(void* at, size_t size)
{
    allocation_t *new_allocation = NULL;

    new_allocation = malloc(sizeof(allocation_t));
    new_allocation->addr = at;
    new_allocation->size = size;

    return new_allocation;
}

void*
allocate(size_t size)
{
    allocation_t *it = NULL, *new_allocation = NULL;
    void *last_endp = (void*) MIN_ALLOC_ADDRESS; /* pointer to the end of the last allocation seen */

    /*
     * If we do not have any allocations yet, just allocate at the first
     * possible address.
     */
    if (unlikely(TAILQ_EMPTY(&allocations))) {
        new_allocation =  _allocation((void*) MIN_ALLOC_ADDRESS, size);
        TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        return (void*) MIN_ALLOC_ADDRESS;
    }

    /* 
     * We never remove anything from the list, so we don't need to use the safe
     * version here.
     */
    TAILQ_FOREACH(it, &allocations, entry) {
        if (last_endp && (it->addr >= last_endp + size)) {
            /* We found a sufficiently large space. */
            new_allocation =  _allocation(last_endp, size);
            TAILQ_INSERT_BEFORE(it, new_allocation, entry);
            return last_endp;
        }
        last_endp = it->addr + it->size;
    }

    /*
     * We arrived at the end of the list without finding a sufficiently
     * large free space. Let's allocate one at the end now.
     */
    new_allocation =  _allocation(last_endp, size);
    TAILQ_INSERT_TAIL(&allocations, _allocation(last_endp, size), entry);
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
        new_allocation = _allocation((void*) at, size);
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
        if ((it->addr + it->size) < at)
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
        new_allocation = _allocation((void*) at, size);

        if (it == TAILQ_FIRST(&allocations))
            TAILQ_INSERT_HEAD(&allocations, new_allocation, entry);
        else
            /* The requested chunk is before the current chunk. */
            TAILQ_INSERT_BEFORE(it, new_allocation, entry);

        return at;
    }

    /* We did not find a chunk that lies after the current chunk. */
    new_allocation = _allocation((void*) at, size);
    TAILQ_INSERT_TAIL(&allocations, new_allocation, entry);
    return at;
}