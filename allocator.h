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

#pragma once

#include <stddef.h>

#define kiB (size_t) (1024)
#define MiB (size_t) (1024 * 1024)
#define GiB (size_t) (1024 * 1024 * 1024)

/**
 * @brief Initialize the guest memory allocator with the memory mapping
 * information.
 *
 * @param lowmem  the size of the lowmem segment at 1 MiB
 * @param highmem the size of the highmem segment above 4 GiB
 */
void init_allocator(size_t lowmem, size_t highmem);

/**
 * @brief Find a free area for a chunk of at least @param size bytes
 * 
 * This is a simple first-fit algorithm.
 * Iterate over allocations until a fitting space is found, then allocate
 * a chunk of @param size bytes at the start of this area.
 * 
 * @param size size in bytes to allocate
 * @return void* pointer to the allocated chunk
 */
void* allocate(size_t size);

/**
 * @brief Attempt to allocate a chunk of @param size bytes at @param at address.
 * 
 * @param at address at which to allocate
 * @param size size in bytes to allocate
 * @return void* pointer to the allocated chunk or NULL if the allocation could
 *               not be performed.
 */
void* allocate_at(void* at, size_t size);