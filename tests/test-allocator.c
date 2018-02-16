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

#include <atf-c.h>
#include <allocator.h>
#include <stdio.h>

ATF_TC(sequential);
ATF_TC_HEAD(sequential, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test simple allocation sequential without a highmem segment.");
}
ATF_TC_BODY(sequential, tc)
{
    void* p = NULL;

    /*
     * Set up a memory mapping from 1 MiB of size 0x1000.
     */
    init_allocator(1 * MiB +0x5000, 0);

    p = allocate(0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB),
                        "Expected first allocation at 1 MiB (%p), "
                        "but pointer returned was at %p.",
                        (void*) (1 * MiB),
                        p);

    p = allocate(0x2000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB)+0x1000,
                        "Expected second allocation at (1 * MiB) + %x "
                        "(%p), but pointer returned was at %p.",
                        0x1000,
                        (void*) (1 * MiB)+0x1000,
                        p);

    p = allocate(0x2000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB)+0x3000,
                        "Expected third allocation at (1 * MiB) + %x "
                        "(%p), but pointer returned was at %p.",
                        0x3000,
                        (void*) (1 * MiB)+0x3000,
                        p);

    /* The following allocation should go out of usable memory, and fail. */
    p = allocate(0x2000);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected allocation at (1 * MiB) + %x "
                        "(%p) to fail, but pointer returned was at %p.",
                        0x5000,
                        (void*) (1 * MiB)+0x5000,
                        p);
}

ATF_TC(allocate_at);
ATF_TC_HEAD(allocate_at, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test the allocate_at allocation");
}
ATF_TC_BODY(allocate_at, tc)
{
    void *p = NULL;

    /*
     * Set up a memory mapping from 1 MiB of size 0x5000.
     */
    init_allocator(1 * MiB + 0x7200, 0);

    /* 
     * Allocate 3 chunks in arbitrary, but not sorted order:
     * 
     * 1 MiB + 0x2200 to 1 MiB + 0x3200
     * 1 MiB + 0x4200 to 1 MiB + 0x5200
     * 1 MiB + 0x6200 to 1 MiB + 0x6200
     */
    p = allocate_at((void*) (1 * MiB) + 0x4200, 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x4200,
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB) +  0x4200,
                        p);

    p = allocate_at((void*) (1 * MiB) + 0x2200, 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x2200,
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB) + 0x2200,
                        p);

    p = allocate_at((void*) (1 * MiB) + 0x6200, 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x6200,
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB) + 0x6200,
                        p);

    /* Attempt to allocate chunk overlapping with previous chunk */
    p = allocate_at((void*) (1 * MiB) + 0x5100, 0x1000);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected allocation at %p to fail, but %p returned.",
                        (void*) (1 * MiB) + 0x5100, p);

    /* Attempt to allocate chunk overlapping with following chunk */
    p = allocate_at((void*) (1 * MiB) + 0x1300, 0x1000);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected allocation at %p to fail, but %p returned.",
                        (void*) (1 * MiB) + 0x1300, p);

    /* Attempt to allocate a chunk adjacent to the previous chunk */
    p = allocate_at((void*) (1 * MiB) + 0x3200, 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x3200,
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB) + 0x3200,
                        p);

    /* Attempt to allocate a chunk adjacent to the following chunk */
    p = allocate_at((void*) (1 * MiB) + 0x5200, 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x5200,
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB) + 0x5200,
                        p);
}

ATF_TC(firstfit);
ATF_TC_HEAD(firstfit, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test the first-fit algorithm");
}
ATF_TC_BODY(firstfit, tc)
{
    void* p = NULL;

    /*
     * Set up a memory mapping from 1 MiB of size 0x5000.
     */
    init_allocator(1 * MiB + 0x4000, 0);
    
    /* Allocate two chunks, leaving a gap between them. */
    allocate_at((void*) (1 * MiB), 0x1000);
    allocate_at((void*) (1 * MiB) + 0x2000, 0x1000);

    /* This allocation should go to (1 * MiB)+0x1000 */
    p = allocate(0x100);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x1000,
                        "Expected allocation at %p, but pointer returned was at"
                        "%p.",
                        (void*) (1 * MiB) + 0x1000,
                        p);

    /* This allocation should fill the complete gap */
    p = allocate(0x1000-0x100);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x1100,
                        "Expected allocation at %p, but pointer returned was at"
                        "%p.",
                        (void*) (1 * MiB) + 0x1100,
                        p);

    /* The following allocation should go at the end */
    p = allocate(0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB) + 0x3000,
                        "Expected allocation at %p, but pointer returned was at"
                        "%p.",
                        (void*) (1 * MiB) + 0x3000,
                        p);
}

ATF_TC(mmap);
ATF_TC_HEAD(mmap, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test allocations with memory map");
}
ATF_TC_BODY(mmap, tc)
{
    void* p = NULL;

    /*
     * Set up a memory mapping from 1 MiB of size 0x5000.
     */
    init_allocator(1 * MiB + 0x4000, 0x3000);

    /* Too big for the lowmem segment */
    p = allocate(0x5000);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected lowmem allocation of size 0x%x to fail, but "
                        " %p was returned.",
                        0x5000,
                        p);

    /* Fill the lowmem segment. */
    p = allocate(0x4000);
    ATF_CHECK_EQ_MSG(p, (void*) (1 * MiB),
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (1 * MiB),
                        p);

    /* Too big for the highmem segment */
    p = allocate(0x4000);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected highmem allocation of size 0x%x to fail, but "
                        "%p was returned.",
                        0x4000,
                        p);

    p = allocate_at((void*) (4 * GiB + 0x2000), 0x1000);
    ATF_CHECK_EQ_MSG(p, (void*) (4 * GiB + 0x2000),
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (4 * GiB +0x2000),
                        p);

    p = allocate(0x2000);
    ATF_CHECK_EQ_MSG(p, (void*) (4 * GiB),
                        "Expected allocation at %p, but pointer returned was at"
                        " %p.",
                        (void*) (4 * GiB),
                        p);

    /* Now memory should be full */
    p = allocate(1);
    ATF_CHECK_EQ_MSG(p, NULL,
                        "Expected highmem allocation of size 0x%x to fail, but "
                        "%p was returned.",
                        1,
                        p);
}

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, sequential);
    ATF_TP_ADD_TC(tp, allocate_at);
    ATF_TP_ADD_TC(tp, firstfit);
    ATF_TP_ADD_TC(tp, mmap);

    return atf_no_error();
}
