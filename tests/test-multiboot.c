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
#include <sys/random.h>
#include <tests/data-multiboot.h>
#include <tests/mock/bhyveload.h>
#include <allocator.h>
#include <libelf.h>
#include <loader.h>
#include <multiboot.h>
#include <errno.h>
#include <stdio.h>

struct loader_callbacks *callbacks = &mock_callbacks;
void *callbacks_arg = NULL;;

ATF_TC(testdata);
ATF_TC_HEAD(testdata, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test whether multiboot test data exists");
}
ATF_TC_BODY(testdata, tc)
{
    printf("%p", MB_TESTDATA_START(mmap));
    ATF_CHECK_EQ_MSG(0, (NULL == MB_TESTDATA_START(mmap)),
                        "mmap testcase data not found");
    ATF_CHECK_EQ_MSG(0, (NULL == MB_TESTDATA_END(mmap)),
                        "mmap testcase data end not found");
    ATF_CHECK_EQ_MSG(0, (0 < MB_TESTDATA_SIZE(mmap)), "mmap test size");

    ATF_CHECK_EQ_MSG(0, (NULL == MB_TESTDATA_START(modules)),
                        "modules testcase data not found");
    ATF_CHECK_EQ_MSG(0, (NULL == MB_TESTDATA_END(modules)),
                        "modules testcase data end not found");
    ATF_CHECK_EQ_MSG(0, (0 < MB_TESTDATA_SIZE(modules)), "mmap test size");
}

ATF_TC(scan);
ATF_TC_HEAD(scan, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test scanning for a multiboot header");
}
ATF_TC_BODY(scan, tc)
{
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    struct multiboot2_header *mb2h = NULL;
    char *random_buffer = NULL;
    FILE *urandom = NULL;

    mb = mb_scan(MB_TESTDATA_START(mmap), MB_TESTDATA_SIZE(mmap));
    ATF_CHECK_EQ(MULTIBOOT1_MAGIC, mb->magic);
    ATF_CHECK_MSG(mb->info.mb.header >= MB_TESTDATA_START(mmap),
        "mb_scan did not find the header");

    ATF_CHECK(urandom = fopen("/dev/urandom", "r"));

    ATF_CHECK_MSG(random_buffer = malloc(128*kiB), "could not allocate memory");
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));

    /* Random data */
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_MSG(mb == 0, "returned non-NULL for random data");

    /* Craft a valid multiboot header */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 2*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0;
    mbh->checksum = (uint32_t) 0U-MULTIBOOT1_MAGIC;
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_EQ_MSG(mb->info.mb.header, mbh,
                        "did not find crafted valid header");
    mbh = NULL;

    /* Craft a valid multiboot header out of search range*/
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 10*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0;
    mbh->checksum = (uint32_t) 0U-MULTIBOOT1_MAGIC;
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_MSG(!mb, "found valid header out of range");
    mbh = NULL;

    /* Craft a header with an invalid checksum */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 10*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0;
    mbh->checksum = 0xdeadbeef;
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_MSG(!mb, "found valid header with invalid checksum");
    mb = NULL;

    /* Craft a valid multiboot2 header */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mb2h = (struct multiboot2_header* )(random_buffer + 16*kiB);
    mb2h->magic = MULTIBOOT2_MAGIC;
    mb2h->header_length = 0;
    mb2h->architecture = 0;
    mb2h->checksum = (uint32_t) 0U-MULTIBOOT2_MAGIC;
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_EQ(MULTIBOOT2_MAGIC, mb->magic);
    mb2h = NULL;

    /* Craft a valid multiboot2 header out of search range */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mb2h = (struct multiboot2_header* )(random_buffer + 36*kiB);
    mb2h->magic = MULTIBOOT2_MAGIC;
    mb2h->header_length = 0;
    mb2h->architecture = 0;
    mb2h->checksum = (uint32_t) 0U-MULTIBOOT2_MAGIC;
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_MSG(!mb, "found valid header out of range");
    mb2h = NULL;

    free(random_buffer);
    fclose(urandom);
}

ATF_TC(loadtype);
ATF_TC_HEAD(loadtype, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test load type detection");
}
ATF_TC_BODY(loadtype, tc)
{
    char *random_buffer = NULL;
    FILE *urandom = NULL;
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    void *kernel = MB_TESTDATA_START(mmap);
    size_t kernsz = MB_TESTDATA_SIZE(mmap);
    enum LOAD_TYPE type;
    Elf *kernelf;

    mb = mb_scan(kernel, kernsz);

    type = multiboot_load_type(kernel, kernsz, &kernelf, mb->info.mb.header);
    ATF_CHECK_EQ_MSG(type, LOAD_ELF, "mmap test kernel not loaded as ELF");

    ATF_CHECK(urandom = fopen("/dev/urandom", "r"));
    ATF_CHECK_MSG(random_buffer = malloc(128*kiB), "could not allocate memory");
    elf_end(kernelf);

    /* Craft a valid multiboot header */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 2*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0 | MULTIBOOT_AOUT_KLUDGE;
    mbh->checksum = (uint32_t) 0U-MULTIBOOT1_MAGIC-(mbh->flags);
    mb = mb_scan(random_buffer, 128*kiB);
    ATF_CHECK_EQ_MSG(mb->info.mb.header, mbh,
                     "did not find crafted valid header");
    mbh = NULL;
    type = multiboot_load_type(random_buffer, 128*kiB,
                               &kernelf, mb->info.mb.header);
    ATF_CHECK_EQ_MSG(type, LOAD_AOUT,
                     "random test data not loaded as a.out kludge");
    ATF_CHECK_EQ_MSG(NULL, kernelf, "ELF resources not freed");

    free(random_buffer);
    fclose(urandom);
}

ATF_TC(load_aout_direct);
ATF_TC_HEAD(load_aout_direct, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test loading a.out kludge directly");
}
ATF_TC_BODY(load_aout_direct, tc)
{
    char *random_buffer = NULL;
    FILE *urandom = NULL;
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    uint32_t error = 0;
    size_t lowmem = 0;
    size_t highmem = 0;

    setmem(4*MiB, 0);
    callbacks->getmem(callbacks_arg, &lowmem, &highmem);
    init_allocator(lowmem, highmem);

    ATF_CHECK(urandom = fopen("/dev/urandom", "r"));
    ATF_CHECK_MSG(random_buffer = malloc(128*kiB), "could not allocate memory");

    /* Load a kernel with a valid a.out kludge */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 2*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0 | MULTIBOOT_AOUT_KLUDGE;
    mbh->checksum = (uint32_t) 0U-MULTIBOOT1_MAGIC-(mbh->flags);
    mbh->header_addr = 1*MiB;
    mbh->load_addr = 1*MiB;
    mbh->load_end_addr = 1*MiB + 0x100;
    mbh->bss_end_addr = 1*MiB + 0x200;
    mbh->entry_addr = 1*MiB + sizeof(struct multiboot_header);

    mb = mb_scan(random_buffer, 128*kiB);
    error = multiboot_load_aout(random_buffer, 128*kiB, mbh);
    ATF_CHECK_EQ_MSG(0, error, "multiboot_load_aout failed");

    /* load_addr must be less than or equal to header_addr */
    mbh->load_addr = 1*MiB+1;
    mb = mb_scan(random_buffer, 128*kiB);
    error = multiboot_load_aout(random_buffer, 128*kiB, mbh);
    ATF_CHECK_EQ_MSG(EINVAL, error, "load_addr > header_addr");

    free(random_buffer);
    fclose(urandom);
}

ATF_TC(load_aout);
ATF_TC_HEAD(load_aout, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test loading a.out kludge directly");
}
ATF_TC_BODY(load_aout, tc)
{
    char *random_buffer = NULL;
    FILE *urandom = NULL;
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    uint32_t error = 0;
    size_t lowmem = 0;
    size_t highmem = 0;

    setmem(4*MiB, 0);
    callbacks->getmem(callbacks_arg, &lowmem, &highmem);
    init_allocator(lowmem, highmem);

    ATF_CHECK(urandom = fopen("/dev/urandom", "r"));
    ATF_CHECK_MSG(random_buffer = malloc(128*kiB), "could not allocate memory");

    /* Load a kernel with a valid a.out kludge */
    ATF_CHECK(fread(random_buffer, 128*kiB, 1, urandom));
    mbh = (struct multiboot_header* )(random_buffer + 2*kiB);
    mbh->magic = MULTIBOOT1_MAGIC;
    mbh->flags = 0 | MULTIBOOT_AOUT_KLUDGE;
    mbh->checksum = (uint32_t) 0U-MULTIBOOT1_MAGIC-(mbh->flags);
    mbh->header_addr = 1*MiB;
    mbh->load_addr = 1*MiB;
    mbh->load_end_addr = 1*MiB + 0x100;
    mbh->bss_end_addr = 1*MiB + 0x200;
    mbh->entry_addr = 1*MiB + sizeof(struct multiboot_header);

    mb = mb_scan(random_buffer, 128*kiB);
    error = multiboot_load(random_buffer, 128*kiB, mbh);
    ATF_CHECK_EQ_MSG(0, error, "multiboot_load failed");

    free(random_buffer);
    fclose(urandom);
}

ATF_TC(load_elf_direct);
ATF_TC_HEAD(load_elf_direct, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test loading ELF directly");
}
ATF_TC_BODY(load_elf_direct, tc)
{
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    uint32_t error = 0;
    size_t lowmem = 0;
    size_t highmem = 0;
    void *kernel = MB_TESTDATA_START(mmap);
    size_t kernsz = MB_TESTDATA_SIZE(mmap);
    Elf *kernelf;

    setmem(4*MiB, 0);
    callbacks->getmem(callbacks_arg, &lowmem, &highmem);
    init_allocator(lowmem, highmem);

    mb = mb_scan(kernel, kernsz);
    mbh = mb->info.mb.header;
    multiboot_load_type(kernel, kernsz, &kernelf, mbh);
    error = multiboot_load_elf(kernel, kernsz, kernelf);
    ATF_CHECK_EQ_MSG(0, error, "multiboot_load_elf failed");

    elf_end(kernelf);
}

ATF_TC(load_elf);
ATF_TC_HEAD(load_elf, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test loading ELF");
}
ATF_TC_BODY(load_elf, tc)
{
    struct multiboot *mb = NULL;
    struct multiboot_header *mbh = NULL;
    uint32_t error = 0;
    size_t lowmem = 0;
    size_t highmem = 0;
    void *kernel = MB_TESTDATA_START(mmap);
    size_t kernsz = MB_TESTDATA_SIZE(mmap);

    setmem(4*MiB, 0);
    callbacks->getmem(callbacks_arg, &lowmem, &highmem);
    init_allocator(lowmem, highmem);

    mb = mb_scan(kernel, kernsz);
    mbh = mb->info.mb.header;
    error = multiboot_load(kernel, kernsz, mbh);
    ATF_CHECK_EQ_MSG(0, error, "multiboot_load_elf failed");
}


ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, testdata);
    ATF_TP_ADD_TC(tp, scan);
    ATF_TP_ADD_TC(tp, loadtype);
    ATF_TP_ADD_TC(tp, load_aout_direct);
    ATF_TP_ADD_TC(tp, load_aout);
    ATF_TP_ADD_TC(tp, load_elf_direct);
    ATF_TP_ADD_TC(tp, load_elf);

    return atf_no_error();
}
