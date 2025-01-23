#include "common/log.h"
#include "common/panic.h"
#include "core.h"
#include "memory/pmm.h"

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/uefi/efi.h"

#ifdef __ENV_DEVELOPMENT
static void qemu_debug_log(char c) {
    asm volatile("outb %0, %1" : : "a"(c), "Nd"(0x3F8));
}
#endif

EFI_SYSTEM_TABLE *g_x86_64_uefi_efi_system_table;
EFI_HANDLE g_x86_64_uefi_efi_image_handle;

[[noreturn]] EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    g_x86_64_uefi_efi_system_table = system_table;
    g_x86_64_uefi_efi_image_handle = image_handle;

#ifdef __ENV_DEVELOPMENT
    qemu_debug_log('\n');
    log_sink_set(qemu_debug_log);
#endif

    x86_64_cpu_enable_nx();

    // Initialize physical memory
    UINTN map_size = 0;
    EFI_MEMORY_DESCRIPTOR *map = NULL;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
    EFI_STATUS status = system_table->BootServices->GetMemoryMap(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
    if(status == EFI_BUFFER_TOO_SMALL) {
        map_size += descriptor_size * 6;
        status = system_table->BootServices->AllocatePool(EfiBootServicesData, map_size, (void **) &map);
        if(EFI_ERROR(status)) panic("unable to allocate pool for UEFI memory map");
        status = system_table->BootServices->GetMemoryMap(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
        if(EFI_ERROR(status)) panic("unable retrieve the UEFI memory map");
    }

    // TODO: Leave some memory for UEFI
    for(UINTN i = 0; i < map_size; i += descriptor_size) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) ((uintptr_t) map + i);
        pmm_map_type_t type;
        switch(entry->Type) {
            case EfiConventionalMemory:      type = PMM_MAP_TYPE_FREE; break;
            case EfiBootServicesCode:
            case EfiBootServicesData:        type = PMM_MAP_TYPE_ALLOCATED; break;
            case EfiACPIReclaimMemory:       type = PMM_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case EfiACPIMemoryNVS:           type = PMM_MAP_TYPE_ACPI_NVS; break;
            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
            case EfiUnusableMemory:
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
            case EfiPalCode:
            case EfiMaxMemoryType:
            case EfiReservedMemoryType:
            default:                         type = PMM_MAP_TYPE_RESERVED; break;
        }

        if(type == PMM_MAP_TYPE_FREE) {
            EFI_PHYSICAL_ADDRESS address = entry->PhysicalStart;
            status = system_table->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, entry->NumberOfPages, &address);
            if(!EFI_ERROR(status)) {
                pmm_map_add(entry->PhysicalStart, entry->NumberOfPages * PMM_GRANULARITY, PMM_MAP_TYPE_FREE);
                continue;
            }

            for(EFI_PHYSICAL_ADDRESS j = entry->PhysicalStart; j < entry->PhysicalStart + entry->NumberOfPages * PMM_GRANULARITY; j += PMM_GRANULARITY) {
                address = j;
                status = system_table->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, 1, &address);
                if(EFI_ERROR(status)) continue;
                pmm_map_add(j, PMM_GRANULARITY, PMM_MAP_TYPE_FREE);
            }
            continue;
        }

        pmm_map_add(entry->PhysicalStart, entry->NumberOfPages * PMM_GRANULARITY, type);
    }

    core();
}
