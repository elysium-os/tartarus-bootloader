#pragma once
#include <stdint.h>
#include <drivers/disk.h>

void hal_disk_initialize();
bool hal_disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest);
bool hal_disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src);