#pragma once

#include "fs/vfs.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t vaddr;
    uint64_t size;
    uint8_t read    : 1;
    uint8_t write   : 1;
    uint8_t execute : 1;
} elf_region_t;

typedef struct {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t size;
    uint64_t entry;

    size_t count;
    elf_region_t **regions;
} elf_loaded_image_t;

elf_loaded_image_t *elf_load(vfs_node_t *file, void *address_space);
size_t elf_read_section(vfs_node_t *file, const char *section_name, void **data);
