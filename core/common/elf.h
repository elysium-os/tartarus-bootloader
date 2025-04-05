#pragma once

#include "fs/vfs.h"

#include <stdint.h>

typedef struct {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t size;
    uint64_t entry;
} elf_loaded_image_t;

elf_loaded_image_t *elf_load(vfs_node_t *file, void *address_space);
