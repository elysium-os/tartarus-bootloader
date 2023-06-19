#include "core.h"
#include <tartarus.h>
#include <stdint.h>
#include <log.h>
#include <libc.h>
#include <graphics/fb.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <drivers/disk.h>
#include <drivers/acpi.h>
#include <fs/fat.h>
#include <config.h>
#ifdef __AMD64
#include <drivers/lapic.h>
#endif
#include <smp.h>

#ifdef __UEFI
EFI_SYSTEM_TABLE *g_st;
EFI_HANDLE g_imagehandle;

[[noreturn]] EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_st = SystemTable;
    g_imagehandle = ImageHandle;

    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = SystemTable->BootServices->LocateProtocol(&gop_guid, NULL, (void **) &gop);
    if(EFI_ERROR(status)) log_panic("CORE", "Unable to locate GOP protocol");
    status = fb_initialize(gop, 1920, 1080);
    if(EFI_ERROR(status)) log_panic("CORE", "Unable to initialize a GOP");
#endif

#if defined __BIOS && defined __AMD64
typedef int SYMBOL[];

extern SYMBOL __tartarus_start;
extern SYMBOL __tartarus_end;

[[noreturn]] void core() {
    fb_initialize(1920, 1080);
#endif
    pmm_initialize();
#ifdef __AMD64
#ifdef __BIOS
    pmm_convert(TARTARUS_MEMAP_TYPE_USABLE, TARTARUS_MEMAP_TYPE_BOOT_RECLAIMABLE, 0x6000, 0x1000); // Protect initial stack
    pmm_convert(TARTARUS_MEMAP_TYPE_USABLE, TARTARUS_MEMAP_TYPE_BOOT_RECLAIMABLE, (uintptr_t) __tartarus_start, (uintptr_t) __tartarus_end - (uintptr_t) __tartarus_start);
#endif
    void *smp_rsv_page = pmm_alloc_page(PMM_AREA_CONVENTIONAL); // Allocate a page early to get one lower than 0x100000
    if((uintptr_t) smp_rsv_page >= 0x100000) log_panic("CORE", "Unable to reserve a page for SMP");
#endif
    disk_initialize();

    fat_file_t *cfg;
    disk_t *disk = g_disks;
    while(disk) {
        disk_part_t *partition = disk->partitions;
        while(partition) {
            fat_info_t *fat_info = fat_initialize(partition);
            if(fat_info) {
                cfg = config_find(fat_info);
                if(cfg) break;
                heap_free(fat_info);
            }
            partition = partition->next;
        }
        if(cfg) break;
        disk = disk->next;
    }
    if(!cfg) log_panic("CORE", "Could not locate a config file");

    acpi_rsdp_t *rsdp = acpi_find_rsdp();
    if(!rsdp) log_panic("CORE", "Could not locate RSDP");

#ifdef __AMD64
    void *pml4 = vmm_initialize();
    if(!lapic_supported()) log_panic("CORE", "Local APIC not supported");
    acpi_sdt_header_t *madt = acpi_find_table(rsdp, "APIC");
    if(!madt) log_panic("CORE", "No MADT table present");
    uint64_t *woa = smp_initialize_aps(madt, (uintptr_t) smp_rsv_page, pml4);
#endif


    log("NO PROTOCOLS ACTIVE!\n");

    // SystemTable->BootServices->ExitBootServices(ImageHandle, map_key);
    while(true);
}