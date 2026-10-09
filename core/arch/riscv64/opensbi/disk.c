#include "dev/disk.h"

#include "arch/disk.h"
#include "dev/virtio/virtio_blk.h"

void arch_disk_initialize() {
    virtio_blk_initialize();
}

bool arch_disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest) {
    return virtio_blk_read(disk, lba, sector_count, dest);
}

bool arch_disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src) {
    return virtio_blk_write(disk, lba, sector_count, src);
}
