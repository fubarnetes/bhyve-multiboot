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
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <tests/mock/bhyveload.h>

#define kiB (size_t) (1024)
#define MiB (size_t) (1024 * 1024)
#define GiB (size_t) (1024 * 1024 * 1024)

/* copyin into in the highmem is currently not supported by bhyveload. */
#define TEST_HIGHMEM_ALLOCATION 0

/* make sure getc and putc aren't defined as a macro */
#ifdef getc
#undef getc
#endif

#ifdef putc
#undef putc
#endif

void* callbacks_arg = NULL;

ATF_TC(mock_exit);
ATF_TC_HEAD(mock_exit, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock bhyveload exit");
}
ATF_TC_BODY(mock_exit, tc)
{
    ATF_CHECK_EQ_MSG(exited, 0, "should not have exited yet.");
    ATF_CHECK_EQ_MSG(exit_reason, 0, "exit_reason should not be set");
    callbacks.exit(callbacks_arg, EINVAL);
    ATF_CHECK_EQ_MSG(exited, 1, "should have exited.");
    ATF_CHECK_EQ_MSG(exit_reason, EINVAL, "exit_reason should be EINVAL");
}

ATF_TC(mock_getenv);
ATF_TC_HEAD(mock_getenv, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock bhyveload getenv");
}
ATF_TC_BODY(mock_getenv, tc)
{
    int i = 0;
    const char* var;
    addenv("smbios.bios.vendor=BHYVE");
    addenv("boot_serial=1");

    while ( (var = callbacks.getenv(callbacks_arg, i++)) ) {
        switch(i) {
        case 1:
            ATF_CHECK_STREQ("boot_serial=1", var);
            break;
        case 2:
            ATF_CHECK_STREQ("smbios.bios.vendor=BHYVE", var);
            break;
        default:
            atf_tc_fail("expected only two environment variables, "
                        "but at %ith now", i);
        }
    }
}

ATF_TC(mock_lowmemio);
ATF_TC_HEAD(mock_lowmemio, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock bhyveload low memory I/O");
}
ATF_TC_BODY(mock_lowmemio, tc)
{
    int retv = 0;
    size_t lowmem = 0, highmem = 0;

    const char *testdata_lowmem = "this is the lowmem test data.";
    char *outbuf = calloc(1, strlen(testdata_lowmem)+1);
    uint64_t lowmem_test_address = 128 * kiB;

    ATF_CHECK_EQ(0, highmem_buffer);
    ATF_CHECK_EQ(0, lowmem_buffer);

    setmem(256 * kiB, 4 * kiB);

    ATF_CHECK_EQ(0, highmem_buffer == 0);
    ATF_CHECK_EQ(0, lowmem_buffer == 0);

    callbacks.getmem(callbacks_arg, &lowmem, &highmem);

    ATF_CHECK_EQ(256 * kiB, lowmem);
    ATF_CHECK_EQ(4 * kiB, highmem);

    retv = callbacks.copyin(callbacks_arg, testdata_lowmem,
                            lowmem_test_address, strlen(testdata_lowmem)+1);

    ATF_CHECK_EQ_MSG(0, retv, "lowmem copyin failed");

    ATF_CHECK_STREQ(testdata_lowmem, lowmem_buffer + 128 * kiB);

    retv = callbacks.copyout(callbacks_arg, lowmem_test_address, outbuf,
                                strlen(testdata_lowmem)+1);

    ATF_CHECK_STREQ(testdata_lowmem, outbuf);

    free(outbuf);
}

#if TEST_HIGHMEM_ALLOCATION
ATF_TC(mock_highmemio);
ATF_TC_HEAD(mock_highmemio, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock bhyveload high memory I/O");
}
ATF_TC_BODY(mock_highmemio, tc)
{
    int retv = 0;
    size_t lowmem = 0, highmem = 0;

    const char *testdata_highmem = "this is the highmem test data.";
    uint64_t highmem_test_address = 4 * GiB + 128 * kiB;

    ATF_CHECK_EQ(0, highmem_buffer);
    ATF_CHECK_EQ(0, lowmem_buffer);

    setmem(4 * kiB, 256 * kiB);

    ATF_CHECK_EQ(0, highmem_buffer == 0);
    ATF_CHECK_EQ(0, lowmem_buffer == 0);

    callbacks.getmem(callbacks_arg, &lowmem, &highmem);

    ATF_CHECK_EQ(4 * kiB, lowmem);
    ATF_CHECK_EQ(256 * kiB, highmem);

    retv = callbacks.copyin(callbacks_arg, testdata_highmem,
                            highmem_test_address, strlen(testdata_highmem)+1);

    ATF_CHECK_EQ_MSG(0, retv, "highmem copyin failed");

    ATF_CHECK_STREQ(testdata_highmem, highmem_buffer + 128 * kiB);
}
#endif

static void
mkfile(const char* filename, const char* data, size_t size)
{
    FILE* f = fopen(filename, "w");
    fwrite(data, size, 1, f);
    fclose(f);
}

ATF_TC(mock_hostfileio);
ATF_TC_HEAD(mock_hostfileio, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock bhyveload host file I/O");
}
ATF_TC_BODY(mock_hostfileio, tc)
{
    void *filehandle;
    void *dirhandle;
    int retv = 0;
    int mode = 0, uid = 0, gid = 0;
    uint32_t fileno = 0;
    uint8_t type = 0;
    size_t namelen = 0;
    char *name = malloc(256);

    uint64_t size = 0, resid = 0;
    char* buffer = NULL;

    const char *testdata = "this is a test file.";
    mkfile("testfile", testdata, strlen(testdata));
    mkdir("testdirectory", 0700);
    mkfile("testdirectory/aaa", "", 0);
    mkfile("testdirectory/bbbb", "", 0);
    mkfile("testdirectory/ccccc", "", 0);

    retv = callbacks.open(callbacks_arg, "/testfile", &filehandle);
    ATF_CHECK_EQ_MSG(0, retv, "open failed");

    retv = callbacks.isdir(callbacks_arg, filehandle);
    ATF_CHECK_EQ_MSG(0, retv, "isdir failed");

    retv = callbacks.stat(callbacks_arg, filehandle, &mode, &uid, &gid, &size);
    ATF_CHECK_EQ_MSG(0, retv, "stat failed");
    ATF_CHECK_EQ(strlen(testdata), size);

    buffer = calloc(1, size);
    if (!buffer)
            atf_tc_fail("could not allocate memory");

    retv = callbacks.read(callbacks_arg, filehandle, buffer, size, &resid);
    ATF_CHECK_EQ_MSG(0, retv, "read failed");
    ATF_CHECK_STREQ(testdata, buffer);

    if (buffer)
        free(buffer);

    buffer = calloc(1, size);
    if (!buffer)
            atf_tc_fail("could not allocate memory");

    retv = callbacks.seek(callbacks_arg, filehandle, 5, SEEK_SET);
    ATF_CHECK_EQ_MSG(0, retv, "seek failed");

    retv = callbacks.read(callbacks_arg, filehandle, buffer, size, &resid);
    ATF_CHECK_EQ_MSG(0, retv, "read failed");
    ATF_CHECK_STREQ(testdata+5, buffer);

    if (buffer)
        free(buffer);

    retv = callbacks.close(callbacks_arg, filehandle);
    ATF_CHECK_EQ_MSG(0, retv, "close failed");

    retv = callbacks.open(callbacks_arg, "/testdirectory", &dirhandle);
    ATF_CHECK_EQ_MSG(0, retv, "open failed");

    retv = callbacks.isdir(callbacks_arg, dirhandle);
    ATF_CHECK_EQ_MSG(1, retv, "isdir failed");

    retv = callbacks.seek(callbacks_arg, dirhandle, 5, SEEK_SET);
    ATF_CHECK_EQ(EINVAL, retv);

    retv = callbacks.read(callbacks_arg, dirhandle, NULL, size, &resid);
    ATF_CHECK_EQ_MSG(EINVAL, retv, "read failed");

    while (!(retv = callbacks.readdir(callbacks_arg, dirhandle, &fileno,
                                    &type, &namelen, name)))
    {
        switch(namelen) {
            case 1:
                ATF_CHECK_STREQ(".", name);
                break;
            case 2:
                ATF_CHECK_STREQ("..", name);
                break;
            case 3:
                ATF_CHECK_STREQ("aaa", name);
                break;
            case 4:
                ATF_CHECK_STREQ("bbbb", name);
                break;
            case 5:
                ATF_CHECK_STREQ("ccccc", name);
                break;
            default:
                atf_tc_fail("extra file in test directory");
        }
    }

    retv = callbacks.close(callbacks_arg, filehandle);
    ATF_CHECK_EQ_MSG(0, retv, "close failed");
}

#define TESTCASE_NOTIMPLEMENTED(fcn, args...) \
    ATF_TC(mock_ ## fcn); \
    ATF_TC_HEAD(mock_ ## fcn, tc) \
    { atf_tc_set_md_var(tc, "descr", "Test that " #fcn "isn't implemented"); } \
    ATF_TC_BODY(mock_ ## fcn, tc) { \
        (&callbacks)->fcn(callbacks_arg , ##args); \
        ATF_CHECK_EQ_MSG(exited, 1, "should have exited."); \
        ATF_CHECK_EQ_MSG(exit_reason, ENOSYS, "exit_reason should be ENOSYS"); \
    }

ATF_TC(mock_vmmapi);
ATF_TC_HEAD(mock_vmmapi, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock VMMAPI functions");
}
ATF_TC_BODY(mock_vmmapi, tc)
{
    struct vmctx ctx;
    uint32_t access, limit;
    uint64_t rip, base;
    vcpu_reset(&ctx, 0);

    vm_get_register(&ctx, 0, VM_REG_GUEST_RIP, &rip);
    ATF_CHECK_EQ_MSG(0xfff0, rip,
                        "rip not initialized correctly after cpu reset");

    callbacks.vm_set_register(callbacks_arg, 0, VM_REG_GUEST_RIP, 0xDEADBEEF);
    rip = 0;
    vm_get_register(&ctx, 0, VM_REG_GUEST_RIP, &rip);

    ATF_CHECK_EQ_MSG(0xdeadbeef, rip,
                        "setting rip via vm_set_register failed");

    vm_get_desc(&ctx, 0, VM_REG_GUEST_CS, &base, &limit, &access);
    ATF_CHECK_EQ(0xffff0000, base);
    ATF_CHECK_EQ(0xffff, limit);
    ATF_CHECK_EQ(0x0093, access);

    base = limit = access = 0;
    callbacks.vm_set_desc(callbacks_arg, 0, VM_REG_GUEST_CS,
                            0xaaaa0000, 0xcccc, 0x92);

    vm_get_desc(&ctx, 0, VM_REG_GUEST_CS, &base, &limit, &access);
    ATF_CHECK_EQ(0xaaaa0000, base);
    ATF_CHECK_EQ(0xcccc, limit);
    ATF_CHECK_EQ(0x0092, access);
}

ATF_TC(mock_setreg);
ATF_TC_HEAD(mock_setreg, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock setreg callback");
}
ATF_TC_BODY(mock_setreg, tc)
{
    struct vmctx ctx;
    int error = 0;
    uint64_t actual;

    /* Can only set RSP */
    callbacks.setreg(callbacks_arg, 4, 0xdeadbeef);
    ATF_CHECK_EQ_MSG(0, exited, "setreg failed for RSP");

    error = vm_get_register(&ctx, 0, VM_REG_GUEST_RSP, &actual);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual, "wrong RSP value");

    callbacks.setreg(callbacks_arg, 5, 0xdeadc0de);
    ATF_CHECK_EQ_MSG(1, exited, "setreg succeeded");
}

ATF_TC(mock_setmsr);
ATF_TC_HEAD(mock_setmsr, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock setmsr callback");
}
ATF_TC_BODY(mock_setmsr, tc)
{
    struct vmctx ctx;
    int error = 0;
    uint64_t actual;

    /* Can only set EFER */
    callbacks.setmsr(callbacks_arg, MSR_EFER, 0xdeadbeef);
    ATF_CHECK_EQ_MSG(0, exited, "setmsr failed for EFER");

    error = vm_get_register(&ctx, 0, VM_REG_GUEST_EFER, &actual);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual, "wrong EFER value");

    callbacks.setmsr(callbacks_arg, 0, 0xdeadc0d3);
    ATF_CHECK_EQ_MSG(1, exited, "setmsr succeeded");
}

ATF_TC(mock_setcr);
ATF_TC_HEAD(mock_setcr, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock setcr callback");
}
ATF_TC_BODY(mock_setcr, tc)
{
    struct vmctx ctx;
    int error = 0;
    uint64_t actual;

    /* Can only set CR0, CR3, CR4 */
    callbacks.setcr(callbacks_arg, 0, 0xdeadbeef);
    ATF_CHECK_EQ_MSG(0, exited, "setcr failed for CR0");

    error = vm_get_register(&ctx, 0, VM_REG_GUEST_CR0, &actual);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual, "wrong CR0 value");

    callbacks.setcr(callbacks_arg, 3, 0xdeadbeef);
    ATF_CHECK_EQ_MSG(0, exited, "setcr failed for CR3");

    error = vm_get_register(&ctx, 0, VM_REG_GUEST_CR3, &actual);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual, "wrong CR3 value");

    callbacks.setcr(callbacks_arg, 4, 0xdeadbeef);
    ATF_CHECK_EQ_MSG(0, exited, "setcr failed for CR4");

    error = vm_get_register(&ctx, 0, VM_REG_GUEST_CR4, &actual);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual, "wrong CR4 value");

    callbacks.setcr(callbacks_arg, 1, 0xdeadc0d3);
    ATF_CHECK_EQ_MSG(1, exited, "setcr succeeded");
}

ATF_TC(mock_setgdt);
ATF_TC_HEAD(mock_setgdt, tc)
{
    atf_tc_set_md_var(tc, "descr", "Test mock setgdt callback");
}
ATF_TC_BODY(mock_setgdt, tc)
{
    struct vmctx ctx;
    int error = 0;
    uint64_t actual_base;
    uint32_t actual_size;
    uint32_t actual_access;

    callbacks.setgdt(callbacks_arg, 0xdeadbeef, 0x1337);
    ATF_CHECK_EQ_MSG(0, exited, "setgdt failed");

    error = vm_get_desc(&ctx, 0, VM_REG_GUEST_GDTR,
                        &actual_base, &actual_size, &actual_access);
    ATF_CHECK_EQ_MSG(0, error, "vm_get_register failed");
    ATF_CHECK_EQ_MSG(0xdeadbeef, actual_base, "wrong base");
    ATF_CHECK_EQ_MSG(0x1336, actual_size, "wrong size");
    ATF_CHECK_EQ_MSG(0, actual_access, "wrong access");
}

TESTCASE_NOTIMPLEMENTED(getc);
TESTCASE_NOTIMPLEMENTED(putc, 'a');
TESTCASE_NOTIMPLEMENTED(poll);
TESTCASE_NOTIMPLEMENTED(diskread, 0, 0, NULL, 0, NULL);
TESTCASE_NOTIMPLEMENTED(diskioctl, 0, 0, NULL);
TESTCASE_NOTIMPLEMENTED(exec, 0);
TESTCASE_NOTIMPLEMENTED(delay, 0);

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, mock_exit);
    ATF_TP_ADD_TC(tp, mock_getenv);
    ATF_TP_ADD_TC(tp, mock_lowmemio);
#if TEST_HIGHMEM_ALLOCATION
    ATF_TP_ADD_TC(tp, mock_highmemio);
#endif
    ATF_TP_ADD_TC(tp, mock_hostfileio);
    ATF_TP_ADD_TC(tp, mock_getc);
    ATF_TP_ADD_TC(tp, mock_putc);
    ATF_TP_ADD_TC(tp, mock_poll);
    ATF_TP_ADD_TC(tp, mock_diskread);
    ATF_TP_ADD_TC(tp, mock_diskioctl);
    ATF_TP_ADD_TC(tp, mock_exec);
    ATF_TP_ADD_TC(tp, mock_delay);
    ATF_TP_ADD_TC(tp, mock_vmmapi);
    ATF_TP_ADD_TC(tp, mock_setreg);
    ATF_TP_ADD_TC(tp, mock_setmsr);
    ATF_TP_ADD_TC(tp, mock_setcr);
    ATF_TP_ADD_TC(tp, mock_setgdt);

    return atf_no_error();
}