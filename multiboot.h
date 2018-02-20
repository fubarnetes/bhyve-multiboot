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

#include <sys/types.h>
#include <stdlib.h>
#include <libelf.h>

#define MULTIBOOT1_MAGIC 0x1BADB002
#define MULTIBOOT1_BOOTLOADER_MAGICC 0x2BADB002
#define MULTIBOOT2_MAGIC 0xE85250D6

#define MULTIBOOT_AOUT_KLUDGE (1<<16)
#define MULTIBOOT_FLAG_GRAPHICS (1<<2)
#define MULTIBOOT_FLAG_MEMORY (1<<1)
#define MULTIBOOT_FLAG_ALIGN4k (1<<0)

#define MULTIBOOT_FRAMEBUFFER (1<<12)
#define MULTIBOOT_VBE (1<<11)
#define MULTIBOOT_APM_TABLE (1<<10)
#define MULTIBOOT_BOOTLOADER_NAME (1<<9)
#define MULTIBOOT_CONFIG_TABLE (1<<8)
#define MULTIBOOT_DRIVES (1<<7)
#define MULTIBOOT_MMAP (1<<6)
#define MULTIBOOT_SYMS_ELF (1<<5)
#define MULTIBOOT_SYMS_AOUT (1<<4)
#define MULTIBOOT_MODS (1<<3)
#define MULTIBOOT_CMDLINE (1<<2)
#define MULTIBOOT_BOOTDEVICE (1<<1)
#define MULTIBOOT_MEMINFO (1<<0)

#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB 1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2

#define PACKED __attribute__((packed))

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
} PACKED;

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        struct {
            uint32_t num;
            uint32_t size;
            uint32_t addr;
            uint32_t shndx;
        } PACKED elf;
        struct {
            uint32_t tabsize;
            uint32_t strsize;
            uint32_t addr;
            uint32_t reserved;
        } PACKED aout;
    } syms;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    struct {
        uint32_t control_info;
        uint32_t mode_info;
        uint16_t mode;
        uint16_t interface_seg;
        uint16_t interface_off;
        uint16_t interface_len;
    } PACKED vbe;
    struct {
        uint64_t addr;
        uint32_t pitch;
        uint32_t width;
        uint32_t height;
        uint8_t bpp;
        uint8_t type;
        union {
            struct {
                uint32_t addr;
                uint16_t num_colors;
            } PACKED palette;
            struct {
                uint8_t red_field_position;
                uint8_t red_mask_size;
                uint8_t green_field_position;
                uint8_t green_mask_size;
                uint8_t blue_field_position;
                uint8_t blue_mask_size;
            } PACKED rgb;
        } color_info;
    } PACKED framebuffer;
} PACKED;

enum multiboot_mmap_type {
    MULTIBOOT_MMAP_AVAILABLE = 1,
    MULTIBOOT_MMAP_RESERVED,
    MULTIBOOT_MMAP_ACPI_RECLAIMABLE,
    MULTIBOOT_MMAP_NVS,
    MULTIBOOT_MMAP_BADRAM,
};

struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} PACKED;

struct multiboot2_header {
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
} PACKED;

struct multiboot2_tag {
    uint16_t type;
    uint16_t flags;
    uint32_t size;
} PACKED;

struct multiboot {
    uint32_t magic;
    union {
        struct {
            struct multiboot_header* header;
        } mb;
        struct {
            struct multiboot2_header* header;
        } mb2;
    } header;
    struct multiboot_info info;
};

/**
 * @brief find the location of the Multiboot or Multiboot2 header.
 *
 * The Multiboot header must be contained completely within the first 8192
 * bytes of the OS image, and must be longword (32-bit) aligned.
 * The Multiboot2 header must be contained completely within the first 32768
 * bytes of the OS image, and must be 64-bit aligned.
 * 
 * @return struct multiboot* pointer to a multiboot context
 */
struct multiboot* mb_scan(void *kernel, size_t kernsz);

/**
 * @brief Determine how to load the multiboot image.
 *
 * @param kernel     pointer to the kernel
 * @param kernsz     size of the kernel
 * @param kernel_elf pointer to the ELF object to initialize if loading as an
 *                   ELF.
 * @param mb         pointer to the multiboot context
 * @return enum LOAD_TYPE
 */
enum LOAD_TYPE
multiboot_load_type (void* kernel, size_t kernsz, Elf **kernel_elf,
                     struct multiboot *mb);

/**
 * @brief Attempt to load a file as an a.out object.
 *
 * @param kernel    pointer to the kernel
 * @param kernsz    size of the kernel
 * @param mb        pointer to the multiboot context
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t
multiboot_load_aout(void* kernel, size_t kernsz, struct multiboot *mb);

/**
 * @brief Load an ELF object.
 *
 * @param kernel pointer to the kernel
 * @param kernsz size of the kernel
 * @param kernel_elf kernel ELF object
 * @return uint32_t 0 on success, error code on failure.
 */
uint32_t
multiboot_load_elf(void *kernel, size_t kernsz, Elf *kernel_elf);

/**
 * @brief Load a kernel into guest memory.
 * 
 * @param kernel pointer to the kernel buffer
 * @param kernsz size of the kernel buffer
 * @param mb pointer to the multiboot context
 * @return uint32_t 0 on success, error code on failure.
 */
uint32_t
multiboot_load(void* kernel, size_t kernsz, struct multiboot *mb);

/**
 * @brief Set the multiboot info memory information
 *
 * @param info pointer to the multiboot_info struct
 * @param mem_lower the amount of lower memory starting at 0
 * @param mem_upper the amount of upper memory starting at 1 MiB
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t
multiboot_info_set_meminfo(struct multiboot_info* info,
                           uint32_t mem_lower, uint32_t mem_upper);

/**
 * @brief Set up the e820 memory map and reference it in the multiboot
 *        information structure.
 *
 * @note this function has to be called after @fn multiboot_info_set_mnminfo.
 *
 * @param info pointer to the multiboot_info struct
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t multiboot_info_set_mmap(struct multiboot_info* info);

/**
 * @brief Set multiboot info bootloader name
 *
 * @param info pointer to the multiboot_info struct
 * @param name bootloader name to set
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t
multiboot_info_set_loader_name(struct multiboot_info* info, const char* name);

/**
 * @brief Set multiboot info command line
 *
 * @param info pointer to the multiboot_info struct
 * @param cmdline command line to set
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t
multiboot_info_set_cmdline(struct multiboot_info* info, const char* cmdline);

/**
 * @brief Copy the multiboot structure into the hypervisor and set up registers
 *
 * @param mb pointer to the multiboot context
 * @return uint32_t 0 on success, error code on failure
 */
uint32_t multiboot_info_finalize(struct multiboot *mb);

/* vim: set noexpandtab ts=4 : */ 