#pragma once
#include <stdint.h>

typedef struct disk_part {
    uint32_t id;
    struct disk *disk;
    uint64_t lba;
    uint64_t size;
    struct disk_part *next;
} disk_part_t;

typedef struct disk {
    uint32_t id;
    uint64_t sector_count;
    uint16_t sector_size;
    uint16_t optimal_transfer_size;
    bool read_only;
    struct disk_part *partitions;
    struct disk *next;
} disk_t;

extern disk_t *g_disks;

void disk_initialize_partitions(disk_t *disk);
void disk_read(disk_part_t *part, uint64_t offset, uint64_t count, void *dest);