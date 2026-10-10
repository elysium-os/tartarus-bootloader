#pragma once

#include "dev/disk.h"

#include <stdbool.h>
#include <stdint.h>

void virtio_blk_initialize(void);
bool virtio_blk_read(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest);
bool virtio_blk_write(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src);
