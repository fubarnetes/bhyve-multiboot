#pragma once

#define MULTIBOOT1_MAGIC 0x1BADB002
#define MULTIBOOT2_MAGIC 0xE85250D6
#include <sys/types.h>
#include <stdlib.h>

struct multiboot_header {
	uint32_t magic;
	uint32_t flags;
	uint32_t checksum;
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