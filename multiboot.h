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