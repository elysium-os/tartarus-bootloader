#pragma once

#include "dev/disk.h"

#include <stdint.h>

void arch_disk_initialize();
bool arch_disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest);
bool arch_disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src);