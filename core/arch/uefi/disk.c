#include "dev/disk.h"

#include "common/panic.h"
#include "lib/container.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include "arch/uefi/uefi.h"

#define UEFI_DISK(DISK) (CONTAINER_OF((DISK), uefi_disk_t, common))

typedef struct {
    disk_t common;
    EFI_BLOCK_IO *io;
} uefi_disk_t;

void arch_disk_initialize() {
    UINTN buffer_size = 0;
    EFI_HANDLE *buffer = NULL;
    EFI_GUID guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_STATUS status = g_uefi_system_table->BootServices->LocateHandle(ByProtocol, &guid, NULL, &buffer_size, buffer);
    if(status == EFI_BUFFER_TOO_SMALL) {
        buffer = heap_alloc(buffer_size);
        status = g_uefi_system_table->BootServices->LocateHandle(ByProtocol, &guid, NULL, &buffer_size, buffer);
    }
    if(EFI_ERROR(status)) panic("failed to retrieve block I/O handles");

    for(UINTN i = 0; i < buffer_size; i += sizeof(EFI_HANDLE)) {
        EFI_BLOCK_IO *io;
        status = g_uefi_system_table->BootServices->OpenProtocol(*buffer++, &guid, (void **) &io, g_uefi_image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        if(EFI_ERROR(status) || !io || io->Media->LastBlock == 0 || io->Media->LogicalPartition) continue;

        io->Media->WriteCaching = false;

        uefi_disk_t *disk = heap_alloc(sizeof(uefi_disk_t));
        disk->common.id = io->Media->MediaId;
        disk->io = io;
        disk->common.read_only = io->Media->ReadOnly;
        disk->common.sector_size = io->Media->BlockSize;
        disk->common.sector_count = io->Media->LastBlock + 1;
        disk->common.partitions = 0;
        disk_initialize_partitions(&disk->common);

        disk->common.next = g_disks;
        g_disks = &disk->common;
    }
}

bool arch_disk_read_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *dest) {
    UINTN buffer_size = sector_count * UEFI_DISK(disk)->io->Media->BlockSize;
    void *buffer = pmm_alloc(PMM_AREA_STANDARD, MATH_DIV_CEIL(buffer_size, PMM_GRANULARITY));
    EFI_STATUS status = UEFI_DISK(disk)->io->ReadBlocks(UEFI_DISK(disk)->io, UEFI_DISK(disk)->io->Media->MediaId, lba, buffer_size, buffer);
    memcpy(dest, buffer, buffer_size);
    pmm_free(buffer, MATH_DIV_CEIL(buffer_size, PMM_GRANULARITY));
    return EFI_ERROR(status);
}

bool arch_disk_write_sector(disk_t *disk, uint64_t lba, uint64_t sector_count, void *src) {
    UINTN buffer_size = sector_count * UEFI_DISK(disk)->io->Media->BlockSize;
    void *buffer = pmm_alloc(PMM_AREA_STANDARD, MATH_DIV_CEIL(buffer_size, PMM_GRANULARITY));
    memcpy(buffer, src, buffer_size);
    EFI_STATUS status = UEFI_DISK(disk)->io->WriteBlocks(UEFI_DISK(disk)->io, UEFI_DISK(disk)->io->Media->MediaId, lba, buffer_size, buffer);
    pmm_free(buffer, MATH_DIV_CEIL(buffer_size, PMM_GRANULARITY));
    return EFI_ERROR(status);
}
