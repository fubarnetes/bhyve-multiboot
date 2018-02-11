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

#define MULTIBOOT1_MAGIC 0x1BADB002
#define MULTIBOOT1_BOOTLOADER_MAGICC 0x2BADB002
#define MULTIBOOT2_MAGIC 0xE85250D6
#include <sys/types.h>
#include <stdlib.h>

#define MULTIBOOT_AOUT_KLUDGE (1<<16)
#define MULTIBOOT_FLAG_GRAPHICS (1<<2)
#define MULTIBOOT_FLAG_MEMORY (1<<1)
#define MULTIBOOT_FLAG_ALIGN4k (1<<0)

struct multiboot_header {
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
    uint32_t mode_type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
};

struct multiboot2_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
};

struct multiboot2_tag {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
};

struct multiboot {
    uint32_t magic;
    union {
        struct {
            struct multiboot_header* header;
        } mb;
        struct {
            struct multiboot2_header* header;
        } mb2;
    } info;
};