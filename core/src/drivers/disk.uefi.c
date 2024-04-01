#include "disk.h"
#include <lib/container.h>
#include <common/log.h>
#include <sys/efi.uefi.h>
#include <memory/heap.h>

#define UEFI_DISK(DISK) (CONTAINER_OF((DISK), uefi_disk_t, common))

typedef struct {
    disk_t common;
    EFI_BLOCK_IO *io;
} uefi_disk_t;

void disk_initialize() {
    UINTN buffer_size = 0;
    EFI_HANDLE *buffer = NULL;
    EFI_GUID guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_STATUS status = g_system_table->BootServices->LocateHandle(ByProtocol, &guid, NULL, &buffer_size, buffer);
    if(status == EFI_BUFFER_TOO_SMALL) {
        buffer = heap_alloc(buffer_size);
        status = g_system_table->BootServices->LocateHandle(ByProtocol, &guid, NULL, &buffer_size, buffer);
    }
    if(EFI_ERROR(status)) log_panic("DISK", "Failed to retrieve block I/O handles");

    for(UINTN i = 0; i < buffer_size; i += sizeof(EFI_HANDLE)) {
        EFI_BLOCK_IO *io;
        status = g_system_table->BootServices->OpenProtocol(*buffer++, &guid, (void **) &io, g_imagehandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if(EFI_ERROR(status) || !io || io->Media->LastBlock == 0 || io->Media->LogicalPartition) continue;

        io->Media->WriteCaching = false;

        uefi_disk_t *disk = heap_alloc(sizeof(uefi_disk_t));
        disk->common.id = io->Media->MediaId;
        disk->io = io;
        disk->common.writable = !io->Media->ReadOnly;
        disk->common.sector_size = io->Media->BlockSize;
        disk->common.sector_count = io->Media->LastBlock + 1;
        disk->common.partitions = 0;
        disk_initialize_partitions(&disk->common);

        disk->common.next = g_disks;
        g_disks = &disk->common;
    }
}

bool disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest) {
    return EFI_ERROR(UEFI_DISK(disk)->io->ReadBlocks(UEFI_DISK(disk)->io, UEFI_DISK(disk)->io->Media->MediaId, lba, sector_count * disk->sector_size, dest));
}

bool disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src) {
    return EFI_ERROR(UEFI_DISK(disk)->io->WriteBlocks(UEFI_DISK(disk)->io, UEFI_DISK(disk)->io->Media->MediaId, lba, sector_count * disk->sector_size, src));
}

