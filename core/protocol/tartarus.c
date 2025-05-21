#include "arch/acpi.h"
#include "arch/fb.h"
#include "arch/time.h"
#include "arch/vm.h"
#include "common/config.h"
#include "common/elf.h"
#include "common/log.h"
#include "common/panic.h"
#include "fs/vfs.h"
#include "lib/math.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include <tartarus.h>

#ifdef __ARCH_X86_64
#include "arch/x86_64/cpu.h"
#include "arch/x86_64/lapic.h"
#include "arch/x86_64/smp.h"
#endif

#ifdef __PLATFORM_X86_64_UEFI
#include "arch/x86_64/uefi/uefi.h"
#endif

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

#define STACK_SIZE 16

#define HHDM_MIN_SIZE 0x100000000
#define HHDM_OFFSET 0xFFFF800000000000
#define HHDM_FLAGS (VM_FLAG_READ | VM_FLAG_WRITE | VM_FLAG_EXEC)
#define HHDM_CAST(TYPE, ADDRESS) ((__TARTARUS_PTR(TYPE))((uint64_t) (uintptr_t) (ADDRESS) + HHDM_OFFSET))

extern void protocol_tartarus_handoff(uint64_t entry, void *stack, uint64_t boot_info);

[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel_node, fb_t *fb) {
    log(LOG_LEVEL_INFO, "Tartarus Protocol Version %u.%u", MAJOR_VERSION, MINOR_VERSION);

    // Setup HHDM
    void *address_space = arch_vm_create_address_space();
    arch_vm_map(address_space, VM_PAGE_GRANULARITY, VM_PAGE_GRANULARITY, HHDM_MIN_SIZE - VM_PAGE_GRANULARITY, HHDM_FLAGS);
    arch_vm_map(address_space, VM_PAGE_GRANULARITY, HHDM_OFFSET + VM_PAGE_GRANULARITY, HHDM_MIN_SIZE - VM_PAGE_GRANULARITY, HHDM_FLAGS);

    uint64_t hhdm_size = HHDM_MIN_SIZE;
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].base + g_pmm_map[i].length < HHDM_MIN_SIZE) continue;

        uint64_t base = g_pmm_map[i].base;
        uint64_t length = g_pmm_map[i].length;
        if(base < HHDM_MIN_SIZE) {
            length -= HHDM_MIN_SIZE - base;
            base = HHDM_MIN_SIZE;
        }
        if(base % VM_PAGE_GRANULARITY != 0) {
            length += base % VM_PAGE_GRANULARITY;
            base -= base % VM_PAGE_GRANULARITY;
        }
        if(length % VM_PAGE_GRANULARITY != 0) length += VM_PAGE_GRANULARITY - length % VM_PAGE_GRANULARITY;
        if(base + length > hhdm_size) hhdm_size = base + length;
        arch_vm_map(address_space, base, base, length, HHDM_FLAGS);
        arch_vm_map(address_space, base, HHDM_OFFSET + base, length, HHDM_FLAGS);
    }

// FIX: this is just a hack to fix <1GB mappings breaking 4kb ones
//      bios fb will be under the 4GB mapping
//      but really we should check whether this conflicts with regions or not
#ifdef __PLATFORM_X86_64_UEFI
    arch_vm_map(
        address_space,
        (uint64_t) (uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY),
        (uint64_t) (uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY),
        MATH_CEIL(fb->pitch * fb->height, VM_PAGE_GRANULARITY),
        HHDM_FLAGS
    );
    arch_vm_map(
        address_space,
        (uint64_t) (uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY),
        (uint64_t) ((uintptr_t) MATH_FLOOR(fb->address, VM_PAGE_GRANULARITY)) + HHDM_OFFSET,
        MATH_CEIL(fb->size, VM_PAGE_GRANULARITY),
        HHDM_FLAGS
    );
#endif
    log(LOG_LEVEL_INFO, "HHDM mapped at offset %#llx (of size %#llx)", HHDM_OFFSET, hhdm_size);

    // Load kernel
    elf_loaded_image_t *kernel = elf_load(kernel_node, address_space);
    if(kernel == NULL) panic("failed to load kernel");
    log(LOG_LEVEL_INFO, "Kernel loaded");

    // Load modules
    size_t module_count = config_key_count(config, "module", CONFIG_ENTRY_TYPE_STRING);
    tartarus_module_t *modules = heap_alloc(sizeof(tartarus_module_t) * module_count);
    for(size_t i = 0; i < module_count; i++) {
        const char *module_path = config_find_string_at(config, "module", NULL, i);
        if(module_path == NULL) continue;

        vfs_node_t *module_node = vfs_lookup(kernel_node->vfs, module_path);
        if(module_node == NULL) {
            log(LOG_LEVEL_WARN, "module %s not found", module_path);
            continue;
        }

        size_t module_size = module_node->ops->get_size(module_node);
        void *module_addr = pmm_alloc(PMM_AREA_STANDARD, MATH_DIV_CEIL(module_size, PMM_GRANULARITY));
        if(module_node->ops->read(module_node, module_addr, 0, module_size) != module_size) {
            pmm_free(module_addr, MATH_DIV_CEIL(module_size, PMM_GRANULARITY));
            log(LOG_LEVEL_WARN, "failed to load module %s", module_path);
            continue;
        }

        char *module_name = heap_alloc(string_length(module_path) + 1);
        string_copy(module_name, module_path);
        modules[i].name = HHDM_CAST(char *, module_name);
        modules[i].paddr = (uint64_t) (uintptr_t) module_addr;
        modules[i].size = module_size;

        log(LOG_LEVEL_INFO, "Loaded module %s at %#lx (of size %#lx)", module_path, (uintptr_t) module_addr, module_size);
    }

    // Find ACPI
    acpi_rsdp_t *rsdp = NULL;
    if(config_find_bool(config, "find_rsdp", true)) {
        rsdp = arch_acpi_find_rsdp();
        if(rsdp == NULL) log(LOG_LEVEL_WARN, "could not locate ACPI RSDP");
    }

    // Allocate stack
    void *stack = pmm_alloc(PMM_AREA_STANDARD, STACK_SIZE) + (STACK_SIZE * PMM_GRANULARITY);

    // Setup boot info
    tartarus_boot_info_t *boot_info = heap_alloc(sizeof(tartarus_boot_info_t));
    boot_info->version = ((uint16_t) MAJOR_VERSION << 8) | MINOR_VERSION;

    boot_info->boot_timestamp = arch_time();

    boot_info->kernel.paddr = kernel->paddr;
    boot_info->kernel.vaddr = kernel->vaddr;
    boot_info->kernel.size = kernel->size;

    boot_info->hhdm.offset = HHDM_OFFSET;
    boot_info->hhdm.size = hhdm_size;

    boot_info->acpi_rsdp_address = (uint64_t) (uintptr_t) rsdp;

    tartarus_framebuffer_t *framebuffer = heap_alloc(sizeof(tartarus_framebuffer_t));
    framebuffer->address = HHDM_CAST(void *, fb->address);
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

    boot_info->framebuffer_count = 1;
    boot_info->framebuffers = HHDM_CAST(tartarus_framebuffer_t *, framebuffer);

    boot_info->module_count = module_count;
    boot_info->modules = HHDM_CAST(tartarus_module_t *, modules);

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

    if(cpus != NULL) {
        uint8_t cpu_count = 0;
        for(x86_64_smp_cpu_t *cpu = cpus; cpu; cpu = cpu->next) cpu_count++;

        uint8_t bsp_index = 0;
        tartarus_cpu_t *cpu_array = heap_alloc(sizeof(tartarus_cpu_t) * cpu_count);
        x86_64_smp_cpu_t *cpu = cpus;
        for(uint16_t i = 0; i < cpu_count; i++, cpu = cpu->next) {
            if(cpu->is_bsp) {
                bsp_index = i;
                cpu_array[i].wake_on_write = (__TARTARUS_PTR(uint64_t *)) 0;
            } else {
                cpu_array[i].wake_on_write = HHDM_CAST(uint64_t *, cpu->wake_on_write);
            }
            cpu_array[i].lapic_id = cpu->lapic_id;
            cpu_array[i].initialization_failed = cpu->init_failed;
        }
        boot_info->cpu_count = cpu_count;
        boot_info->cpus = HHDM_CAST(tartarus_cpu_t *, cpu_array);
        boot_info->bsp_index = bsp_index;
    } else {
        tartarus_cpu_t *bsp_cpu = heap_alloc(sizeof(tartarus_cpu_t));
        bsp_cpu->lapic_id = x86_64_lapic_id();
        bsp_cpu->initialization_failed = false;
        bsp_cpu->wake_on_write = (__TARTARUS_PTR(uint64_t *)) 0;
        boot_info->cpu_count = 1;
        boot_info->cpus = HHDM_CAST(tartarus_cpu_t *, bsp_cpu);
        boot_info->bsp_index = 0;
    }
#endif

    tartarus_memory_map_entry_t *memory_map_entries = heap_alloc(sizeof(tartarus_memory_map_entry_t) * g_pmm_map_size);
    for(uint16_t i = 0; i < g_pmm_map_size; i++) {
        memory_map_entries[i].base = g_pmm_map[i].base;
        memory_map_entries[i].length = g_pmm_map[i].length;
        tartarus_memory_map_type_t type;
        switch(g_pmm_map[i].type) {
            case PMM_MAP_TYPE_FREE:             type = TARTARUS_MEMORY_MAP_TYPE_USABLE; break;
            case PMM_MAP_TYPE_ALLOCATED:        type = TARTARUS_MEMORY_MAP_TYPE_BOOT_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_RECLAIMABLE: type = TARTARUS_MEMORY_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_NVS:         type = TARTARUS_MEMORY_MAP_TYPE_ACPI_NVS; break;
            case PMM_MAP_TYPE_BAD:              type = TARTARUS_MEMORY_MAP_TYPE_BAD; break;
            case PMM_MAP_TYPE_RESERVED:
            default:                            type = TARTARUS_MEMORY_MAP_TYPE_RESERVED; break;
        }
        memory_map_entries[i].type = type;
    }
    boot_info->memory_map.entries = HHDM_CAST(tartarus_memory_map_entry_t *, memory_map_entries);
    boot_info->memory_map.size = g_pmm_map_size;

    arch_vm_load_address_space(address_space);

    protocol_tartarus_handoff(kernel->entry, stack, HHDM_CAST(uint64_t, boot_info));
    __builtin_unreachable();
}
