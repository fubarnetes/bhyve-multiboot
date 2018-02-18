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
#include <tests/data-multiboot.h>
#include <stdio.h>

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

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, testdata);

    return atf_no_error();
}