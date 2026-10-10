#pragma once

#include <stdint.h>

struct disk;

typedef struct disk_ops {
    bool (*read_sector)(struct disk *disk, uint64_t lba, uint64_t sector_count, void *dest);
    bool (*write_sector)(struct disk *disk, uint64_t lba, uint64_t sector_count, void *src);
} disk_ops_t;

typedef struct disk_part {
    uint32_t id;
    struct disk *disk;
    uint64_t lba;
    uint64_t size;
    struct disk_part *next;
} disk_part_t;

typedef struct disk {
    uint32_t id;
    bool read_only;
    uint64_t sector_count;
    uint16_t sector_size;
    uint16_t optimal_transfer_size;
    const disk_ops_t *ops;
    struct disk_part *partitions;
    struct disk *next;
} disk_t;

extern disk_t *g_disks;

bool disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest);
bool disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src);

void disk_initialize_partitions(disk_t *disk);
void disk_read(disk_part_t *part, uint64_t offset, uint64_t count, void *dest);
