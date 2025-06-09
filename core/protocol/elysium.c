#include "arch/acpi.h"
#include "arch/fb.h"
#include "arch/time.h"
#include "arch/vm.h"
#include "common/config.h"
#include "common/elf.h"
#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi.h"
#include "elyboot.h"
#include "lib/math.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#ifdef __ARCH_X86_64
#include "arch/x86_64/cpu.h"
#include "arch/x86_64/smp.h"
#endif

#ifdef __PLATFORM_X86_64_UEFI
#include "arch/x86_64/uefi/uefi.h"
#endif

#define MAJOR_VERSION 1
#define MINOR_VERSION 0

#define HHDM_OFFSET 0xFFFF800000000000
#define HHDM_CAST(TYPE, ADDRESS) ((ELYBOOT_PTR(TYPE))((uint64_t) (uintptr_t) (ADDRESS) + HHDM_OFFSET))

#define STACK_PGCNT 16

extern void protocol_elysium_handoff(uint64_t entry, ELYBOOT_PTR(void *) stack, ELYBOOT_PTR(elyboot_t *) boot_info);

[[noreturn]] void protocol_elysium(config_t *config, vfs_node_t *kernel_node, fb_t *fb) {
    log(LOG_LEVEL_INFO, "Elysium Protocol Version %u.%u", MAJOR_VERSION, MINOR_VERSION);

    void *address_space = arch_vm_create_address_space();

    // Load kernel
    elf_loaded_image_t *kernel = elf_load(kernel_node, address_space);
    if(kernel == NULL) panic("Failed to load kernel");
    log(LOG_LEVEL_INFO, "Kernel loaded");

    elyboot_kernel_info_t *kernel_info;
    size_t kernel_info_size = elf_read_section(kernel_node, ".elyboot", (void **) &kernel_info);
    if(kernel_info_size == 0) panic("Failed to read elyboot kernel info");
    if(kernel_info_size < sizeof(elyboot_kernel_info_t)) panic("Elyboot kernel info size mismatch");

    // Freeze the memory map
    size_t frozen_map_size = g_pmm_map_size;
    pmm_map_entry_t *frozen_map = heap_alloc(sizeof(pmm_map_entry_t) * frozen_map_size);
    memcpy(frozen_map, &g_pmm_map, sizeof(pmm_map_entry_t) * frozen_map_size);

    // Setup HHDM
    uint64_t hhdm_size = 0;
    for(size_t i = 0; i < frozen_map_size; i++) {
        switch(frozen_map[i].type) {
            case PMM_MAP_TYPE_FREE:
            case PMM_MAP_TYPE_ALLOCATED:
            case PMM_MAP_TYPE_EFI_RECLAIMABLE:
            case PMM_MAP_TYPE_ACPI_RECLAIMABLE:
            case PMM_MAP_TYPE_RESERVED:         break;
            default:                            continue;
        }

        uint64_t base = frozen_map[i].base;
        uint64_t length = frozen_map[i].length;
        if(base % VM_PAGE_GRANULARITY != 0) {
            length += base % VM_PAGE_GRANULARITY;
            base -= base % VM_PAGE_GRANULARITY;
        }
        if(length % VM_PAGE_GRANULARITY != 0) length += VM_PAGE_GRANULARITY - length % VM_PAGE_GRANULARITY;

        if(base + length > hhdm_size) hhdm_size = base + length;

        arch_vm_map(address_space, base, base, length, VM_FLAG_READ | VM_FLAG_WRITE | VM_FLAG_EXEC);
        arch_vm_map(address_space, base, HHDM_OFFSET + base, length, VM_FLAG_READ | VM_FLAG_WRITE);
    }

    // Map the framebuffer into the HHDM
    arch_vm_map(
        address_space,
        (uint64_t) (uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY),
        (uint64_t) ((uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY)) + HHDM_OFFSET,
        MATH_CEIL(fb->size, VM_PAGE_GRANULARITY),
        VM_FLAG_READ | VM_FLAG_WRITE
    );

    // Setup page cache
    uint64_t page_cache_start = HHDM_OFFSET + hhdm_size;
    uint64_t page_cache_end = page_cache_start;
    for(size_t i = 0; i < frozen_map_size; i++) {
        pmm_map_entry_t *entry = &frozen_map[i];
        switch(entry->type) {
            case PMM_MAP_TYPE_FREE:
            case PMM_MAP_TYPE_ALLOCATED:
            case PMM_MAP_TYPE_EFI_RECLAIMABLE:
            case PMM_MAP_TYPE_ACPI_RECLAIMABLE: break;
            default:                            continue;
        }

        uintptr_t start = page_cache_start + MATH_FLOOR((entry->base / PMM_GRANULARITY) * kernel_info->page_struct_size, PMM_GRANULARITY);
        uintptr_t end = page_cache_start + MATH_CEIL(((entry->base + entry->length) / PMM_GRANULARITY) * kernel_info->page_struct_size, PMM_GRANULARITY);
        for(page_cache_end = start; page_cache_end < end; page_cache_end += PMM_GRANULARITY) {
            arch_vm_map(address_space, (uintptr_t) pmm_alloc(PMM_AREA_STANDARD, 1), page_cache_end, PMM_GRANULARITY, VM_FLAG_READ | VM_FLAG_WRITE);
        }
    }
    uint64_t page_cache_size = page_cache_end - page_cache_start;

    heap_free(frozen_map);

    // Load modules
    size_t module_count = config_key_count(config, "module", CONFIG_ENTRY_TYPE_STRING);
    elyboot_module_t *modules = heap_alloc(sizeof(elyboot_module_t) * module_count);
    for(size_t i = 0; i < module_count; i++) {
        const char *module_path = config_find_string_at(config, "module", NULL, i);
        if(module_path == NULL) continue;

        vfs_node_t *module_node = vfs_lookup(kernel_node->vfs, module_path);
        if(module_node == NULL) {
            log(LOG_LEVEL_WARN, "Module %s not found", module_path);
            continue;
        }

        size_t module_size = module_node->ops->get_size(module_node);
        void *module_addr = pmm_alloc(PMM_AREA_STANDARD, MATH_DIV_CEIL(module_size, PMM_GRANULARITY));
        if(module_node->ops->read(module_node, module_addr, 0, module_size) != module_size) {
            pmm_free(module_addr, MATH_DIV_CEIL(module_size, PMM_GRANULARITY));
            log(LOG_LEVEL_WARN, "Failed to load module %s", module_path);
            continue;
        }

        char *module_name = heap_alloc(string_length(module_path) + 1);
        string_copy(module_name, module_path);
        modules[i].name = HHDM_CAST(const char *, module_name);
        modules[i].paddr = (uint64_t) (uintptr_t) module_addr;
        modules[i].size = module_size;

        log(LOG_LEVEL_INFO, "Loaded module %s at %#lx (of size %#lx)", module_path, (uintptr_t) module_addr, module_size);
    }

    // Allocate stack
    void *stack = pmm_alloc(PMM_AREA_STANDARD, STACK_PGCNT) + (STACK_PGCNT * PMM_GRANULARITY);

    // Find ACPI
    acpi_rsdp_t *rsdp = NULL;
    if(config_find_bool(config, "find_rsdp", true)) {
        rsdp = arch_acpi_find_rsdp();
        if(rsdp == NULL) log(LOG_LEVEL_WARN, "Could not locate ACPI RSDP");
    }

    // Prepare SMP init
#ifdef __PLATFORM_X86_64_UEFI
    log(LOG_LEVEL_INFO, "Exiting UEFI bootservices");
    x86_64_uefi_efi_bootservices_exit();
#endif

    // Initialize SMP
#ifdef __ARCH_X86_64
    x86_64_smp_cpu_t *cpus = NULL;
    if(config_find_bool(config, "smp", true)) {
        void *smp_reserved_page = pmm_alloc(PMM_AREA_LOWMEM, 1);
        log(LOG_LEVEL_INFO, "APINIT reserved page: %#lx", (uintptr_t) smp_reserved_page);
        if(smp_reserved_page == NULL) panic("unable to reserve SMP initialization page");
        if(!g_x86_64_cpu_lapic_support) panic("LAPIC not supported. LAPIC is required for SMP initialization");

        acpi_sdt_header_t *madt = acpi_find_table(rsdp, "APIC");
        if(madt == NULL) panic("ACPI MADT table not present");
        cpus = x86_64_smp_initialize_aps(madt, smp_reserved_page, address_space);
        log(LOG_LEVEL_INFO, "Initialized SMP");
    }
#endif

    // Setup boot info
    elyboot_kernel_segment_t *kernel_segments = heap_alloc(sizeof(elyboot_kernel_segment_t) * kernel->count);
    for(size_t i = 0; i < kernel->count; i++) {
        uint8_t flags = 0;
        if(kernel->regions[i]->read) flags |= ELYBOOT_KERNEL_SEGMENT_FLAG_READ;
        if(kernel->regions[i]->write) flags |= ELYBOOT_KERNEL_SEGMENT_FLAG_WRITE;
        if(kernel->regions[i]->execute) flags |= ELYBOOT_KERNEL_SEGMENT_FLAG_EXECUTE;

        kernel_segments[i].flags = flags;
        kernel_segments[i].vaddr = kernel->regions[i]->aligned_vaddr;
        kernel_segments[i].paddr = kernel->paddr + (kernel->regions[i]->aligned_vaddr - kernel->aligned_vaddr);
        kernel_segments[i].size = kernel->regions[i]->aligned_size;
    }

    elyboot_framebuffer_t *framebuffer = heap_alloc(sizeof(elyboot_framebuffer_t));
    framebuffer->vaddr = HHDM_CAST(void *, fb->address);
    framebuffer->paddr = fb->address;
    framebuffer->size = fb->size;
    framebuffer->width = fb->width;
    framebuffer->height = fb->height;
    framebuffer->pitch = fb->pitch;
    framebuffer->bpp = fb->bpp;
    framebuffer->mask.red_position = fb->mask_red_position;
    framebuffer->mask.red_size = fb->mask_red_size;
    framebuffer->mask.green_position = fb->mask_green_position;
    framebuffer->mask.green_size = fb->mask_green_size;
    framebuffer->mask.blue_position = fb->mask_blue_position;
    framebuffer->mask.blue_size = fb->mask_blue_size;

    elyboot_t *boot_info = heap_alloc(sizeof(elyboot_t));
    boot_info->version = ((uint16_t) MAJOR_VERSION << 8) | MINOR_VERSION;
    boot_info->entry_stack_size = STACK_PGCNT * PMM_GRANULARITY;
    boot_info->kernel_segments = HHDM_CAST(elyboot_kernel_segment_t *, kernel_segments);
    boot_info->kernel_segment_count = kernel->count;
    boot_info->module_count = module_count;
    boot_info->modules = HHDM_CAST(elyboot_module_t *, modules);
    boot_info->framebuffer_count = 1;
    boot_info->framebuffers = HHDM_CAST(elyboot_framebuffer_t *, framebuffer);
    boot_info->acpi_rsdp = (uint64_t) (uintptr_t) rsdp;
    boot_info->hhdm_base = HHDM_OFFSET;
    boot_info->hhdm_size = hhdm_size;
    boot_info->page_cache_base = page_cache_start;
    boot_info->page_cache_size = page_cache_size;

#ifdef __ARCH_X86_64
    if(cpus != NULL) {
        uint8_t cpu_count = 0;
        for(x86_64_smp_cpu_t *cpu = cpus; cpu; cpu = cpu->next) cpu_count++;

        uint32_t sequential_id_counter = 0;
        uint8_t bsp_index = 0;
        elyboot_cpu_t *cpu_array = heap_alloc(sizeof(elyboot_cpu_t) * cpu_count);
        x86_64_smp_cpu_t *cpu = cpus;
        for(uint16_t i = 0; i < cpu_count; i++, cpu = cpu->next) {
            if(cpu->init_failed) {
                cpu_array[i].init_state = ELYBOOT_CPU_STATE_FAIL;
                cpu_array[i].sequential_id = 0;
                cpu_array[i].wake_on_write = (ELYBOOT_PTR(uint64_t *)) 0;
                continue;
            }

            if(cpu->is_bsp) {
                bsp_index = i;
                cpu_array[i].wake_on_write = (ELYBOOT_PTR(uint64_t *)) 0;
            } else {
                cpu_array[i].wake_on_write = HHDM_CAST(uint64_t *, cpu->wake_on_write);
            }

            cpu_array[i].sequential_id = sequential_id_counter++;
            cpu_array[i].init_state = ELYBOOT_CPU_STATE_OK;
        }

        boot_info->bsp_index = bsp_index;
        boot_info->cpu_count = cpu_count;
        boot_info->cpus = HHDM_CAST(elyboot_cpu_t *, cpu_array);
    } else {
        elyboot_cpu_t *bsp_cpu = heap_alloc(sizeof(elyboot_cpu_t));
        bsp_cpu->sequential_id = 0;
        bsp_cpu->init_state = ELYBOOT_CPU_STATE_OK;
        bsp_cpu->wake_on_write = (ELYBOOT_PTR(uint64_t *)) 0;

        boot_info->cpu_count = 1;
        boot_info->cpus = HHDM_CAST(elyboot_cpu_t *, bsp_cpu);
        boot_info->bsp_index = 0;
    }
#endif

    // Create the elysium memory map
    elyboot_mm_entry_t *memory_map_entries = heap_alloc(sizeof(elyboot_mm_entry_t) * g_pmm_map_size);
    for(uint16_t i = 0; i < g_pmm_map_size; i++) {
        elyboot_mm_type_t type;
        switch(g_pmm_map[i].type) {
            case PMM_MAP_TYPE_FREE:             type = ELYBOOT_MM_TYPE_FREE; break;
            case PMM_MAP_TYPE_ALLOCATED:        type = ELYBOOT_MM_TYPE_BOOTLOADER_RECLAIMABLE; break;
            case PMM_MAP_TYPE_EFI_RECLAIMABLE:  type = ELYBOOT_MM_TYPE_EFI_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_RECLAIMABLE: type = ELYBOOT_MM_TYPE_ACPI_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_NVS:         type = ELYBOOT_MM_TYPE_ACPI_NVS; break;
            case PMM_MAP_TYPE_BAD:              type = ELYBOOT_MM_TYPE_BAD; break;
            case PMM_MAP_TYPE_RESERVED:
            default:                            type = ELYBOOT_MM_TYPE_RESERVED; break;
        }

        memory_map_entries[i].type = type;
        memory_map_entries[i].address = g_pmm_map[i].base;
        memory_map_entries[i].length = g_pmm_map[i].length;
    }

    // Fill in the final fields before handoff
    boot_info->mm_entry_count = g_pmm_map_size;
    boot_info->mm_entries = HHDM_CAST(elyboot_mm_entry_t *, memory_map_entries);
    boot_info->boot_timestamp = arch_time();

    // Load the address space
    arch_vm_load_address_space(address_space);

    log(LOG_LEVEL_INFO, "Kernel handoff");

    protocol_elysium_handoff(kernel->entry, HHDM_CAST(void *, stack), HHDM_CAST(elyboot_t *, boot_info));
    __builtin_unreachable();
}
