#include <stdint.h>
#include <cpuid.h>
#include <lib/mem.h>
#include <common/log.h>
#include <core.h>
#include <memory/pmm.h>
#include <hal/uefi/efi.h>
#include <hal/x86_64/msr.h>

#define CPUID_NX (1 << 20)
#define MSR_EFER 0xC0000080
#define MSR_EFER_NX (1 << 11)

EFI_SYSTEM_TABLE *g_system_table;
EFI_HANDLE g_imagehandle;

bool g_cpu_nx_support = false;

static void log_sink(char c) {
    asm volatile("outb %0, %1" : : "a" (c), "Nd" (0x3F8));
}

[[noreturn]] EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_system_table = SystemTable;
    g_imagehandle = ImageHandle;

    log_sink('\n');
    log_init(log_sink);
    log("CORE", "Tartarus Bootloader (UEFI)");

    // Enable NX
    unsigned int edx = 0, unused;
    if(__get_cpuid(0x80000001, &unused, &unused, &unused, &edx) != 0) g_cpu_nx_support = (edx & CPUID_NX) != 0;
    if(g_cpu_nx_support) {
        msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_NX);
    } else {
        log_warning("CORE", "CPU does not support EFER.NXE");
    }

    // Initialize physical memory
    UINTN map_size = 0;
    EFI_MEMORY_DESCRIPTOR *map = NULL;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
    EFI_STATUS status = g_system_table->BootServices->GetMemoryMap(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
    if(status == EFI_BUFFER_TOO_SMALL) {
        map_size += descriptor_size * 6;
        status = g_system_table->BootServices->AllocatePool(EfiBootServicesData, map_size, (void **) &map);
        if(EFI_ERROR(status)) log_panic("PMM", "Unable to allocate pool for UEFI memory map");
        status = g_system_table->BootServices->GetMemoryMap(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
        if(EFI_ERROR(status)) log_panic("PMM", "Unable retrieve the UEFI memory map");
    }

    // TODO: Leave some memory for UEFI
    for(UINTN i = 0; i < map_size; i += descriptor_size) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) ((uintptr_t) map + i);
        pmm_map_type_t type;
        switch(entry->Type) {
            case EfiConventionalMemory: type = PMM_MAP_TYPE_FREE; break;
            case EfiBootServicesCode:
            case EfiBootServicesData: type = PMM_MAP_TYPE_ALLOCATED; break;
            case EfiACPIReclaimMemory: type = PMM_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case EfiACPIMemoryNVS: type = PMM_MAP_TYPE_ACPI_NVS; break;
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
            default: type = PMM_MAP_TYPE_RESERVED; break;
        }

        if(type == PMM_MAP_TYPE_FREE) {
            EFI_PHYSICAL_ADDRESS address = entry->PhysicalStart;
            status = g_system_table->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, entry->NumberOfPages, &address);
            if(!EFI_ERROR(status)) {
                pmm_init_add(entry->PhysicalStart, entry->NumberOfPages * PMM_PAGE_SIZE, PMM_MAP_TYPE_FREE);
                continue;
            }

            for(EFI_PHYSICAL_ADDRESS j = entry->PhysicalStart; j < entry->PhysicalStart + entry->NumberOfPages * PMM_PAGE_SIZE; j += PMM_PAGE_SIZE) {
                address = j;
                status = g_system_table->BootServices->AllocatePages(AllocateAddress, EfiBootServicesData, 1, &address);
                if(EFI_ERROR(status)) continue;
                pmm_init_add(j, PMM_PAGE_SIZE, PMM_MAP_TYPE_FREE);
            }
            continue;
        }

        pmm_init_add(entry->PhysicalStart, entry->NumberOfPages * PMM_PAGE_SIZE, type);
    }

    core();
    __builtin_unreachable();
}