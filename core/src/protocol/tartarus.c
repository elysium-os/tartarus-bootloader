#include "protocol.h"
#include <stdint.h>
#include <common/log.h>
#include <common/elf.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <drivers/acpi.h>
#include <hal/vmm.h>
#include <hal/acpi.h>
#include <tartarus.h>
#ifdef __X86_64
#include <hal/x86_64/lapic.h>
#include <hal/x86_64/smp.h>
#endif

#define HHDM_MIN_SIZE 0x100000000
#define HHDM_OFFSET 0xFFFF800000000000
#define HHDM_FLAGS (VMM_FLAG_READ | VMM_FLAG_WRITE | VMM_FLAG_EXEC)
#define HHDM_CAST(TYPE, ADDRESS) ((__TARTARUS_PTR(TYPE)) ((uint64_t) (uintptr_t) (ADDRESS) + HHDM_OFFSET))

extern void protocol_tartarus_handoff(uint64_t entry, void *stack, uint64_t boot_info);

[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel_node, fb_t fb) {
    // Setup HHDM
    void *address_space = hal_vmm_create_address_space();
    hal_vmm_map(address_space, PMM_PAGE_SIZE, PMM_PAGE_SIZE, HHDM_MIN_SIZE - PMM_PAGE_SIZE, HHDM_FLAGS);
    hal_vmm_map(address_space, PMM_PAGE_SIZE, HHDM_OFFSET + PMM_PAGE_SIZE, HHDM_MIN_SIZE - PMM_PAGE_SIZE, HHDM_FLAGS);

    uint64_t hhdm_size = HHDM_MIN_SIZE;
    for(uint16_t i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].base + g_pmm_map[i].length < HHDM_MIN_SIZE) continue;
        uint64_t base = g_pmm_map[i].base;
        uint64_t length = g_pmm_map[i].length;
        if(base < HHDM_MIN_SIZE) {
            length -= HHDM_MIN_SIZE - base;
            base = HHDM_MIN_SIZE;
        }
        if(base % PMM_PAGE_SIZE != 0) {
            length += base % PMM_PAGE_SIZE;
            base -= base % PMM_PAGE_SIZE;
        }
        if(length % PMM_PAGE_SIZE != 0) length += PMM_PAGE_SIZE - length % PMM_PAGE_SIZE;
        if(base + length > hhdm_size) hhdm_size = base + length;
        hal_vmm_map(address_space, base, base, length, HHDM_FLAGS);
        hal_vmm_map(address_space, base, HHDM_OFFSET + base, length, HHDM_FLAGS);
    }

    hal_vmm_load_address_space(address_space);

    // Load kernel
    elf_loaded_image_t *kernel = elf_load(kernel_node, address_space);
    if(kernel == NULL) log_panic("PROTO_TARTARUS", "Failed to load kernel");

    // Find ACPI
    acpi_rsdp_t *rsdp = hal_acpi_find_rsdp();
    if(rsdp == NULL) log_panic("PROTO_TARTARUS", "Could not locate RSDP");

    // Initialize SMP
#ifdef __X86_64
    smp_cpu_t *cpus = NULL;
    if(config_read_bool(config, "SMP", true)) {
        void *smp_reserved_page = pmm_alloc(PMM_AREA_LOWMEM, 1);
        if(smp_reserved_page == NULL) log_panic("PROTO_TARTARUS", "Unable to reserve SMP init page");
        if(!lapic_is_supported()) log_panic("PROTO_TARTARUS", "LAPIC not supported. LAPIC is required for SMP initialization");
        acpi_sdt_header_t *madt = acpi_find_table(rsdp, "APIC");
        if(madt == NULL) log_panic("PROTO_TARTARUS", "ACPI MADT table not present");
        cpus = smp_initialize_aps(madt, smp_reserved_page, address_space);
        log("PROTO_TARTARUS", "Initialized SMP");
    }
#endif

    // Free config
    config_free(config);

    // Setup boot info
    tartarus_memory_map_entry_t *memory_map_entries = heap_alloc(sizeof(tartarus_memory_map_entry_t) * g_pmm_map_size);
    for(uint16_t i = 0; i < g_pmm_map_size; i++) {
        memory_map_entries[i].base = g_pmm_map[i].base;
        memory_map_entries[i].length = g_pmm_map[i].length;
        tartarus_memory_map_type_t type;
        switch(g_pmm_map[i].type) {
            case PMM_MAP_TYPE_FREE: type = TARTARUS_MEMORY_MAP_TYPE_USABLE; break;
            case PMM_MAP_TYPE_ALLOCATED: type = TARTARUS_MEMORY_MAP_TYPE_BOOT_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_RECLAIMABLE: type = TARTARUS_MEMORY_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case PMM_MAP_TYPE_ACPI_NVS: type = TARTARUS_MEMORY_MAP_TYPE_ACPI_NVS; break;
            case PMM_MAP_TYPE_BAD: type = TARTARUS_MEMORY_MAP_TYPE_BAD; break;
            case PMM_MAP_TYPE_RESERVED:
            default: type = TARTARUS_MEMORY_MAP_TYPE_RESERVED; break;
        }
        memory_map_entries[i].type = type;
    }

    tartarus_boot_info_t *boot_info = heap_alloc(sizeof(tartarus_boot_info_t));
    boot_info->kernel.paddr = kernel->paddr;
    boot_info->kernel.vaddr = kernel->vaddr;
    boot_info->kernel.size = kernel->size;

    boot_info->hhdm.offset = HHDM_OFFSET;
    boot_info->hhdm.size = hhdm_size;

    boot_info->memory_map.entries = HHDM_CAST(tartarus_memory_map_entry_t *, memory_map_entries);
    boot_info->memory_map.size = g_pmm_map_size;

    boot_info->framebuffer.address = HHDM_CAST(void *, fb.address);
    boot_info->framebuffer.size = fb.size;
    boot_info->framebuffer.width = fb.width;
    boot_info->framebuffer.height = fb.height;
    boot_info->framebuffer.pitch = fb.pitch;

    boot_info->acpi_rsdp = HHDM_CAST(void *, rsdp);

#ifdef __X86_64
    uint8_t cpu_count = 0;
    for(smp_cpu_t *cpu = cpus; cpu; cpu = cpu->next) cpu_count++;

    uint8_t bsp_index = 0;
    tartarus_cpu_t *cpu_array = heap_alloc(sizeof(tartarus_cpu_t) * cpu_count);
    smp_cpu_t *cpu = cpus;
    for(uint16_t i = 0; i < cpu_count; i++, cpu = cpu->next) {
        if(cpu->is_bsp) {
            bsp_index = i;
            cpu_array[i].wake_on_write = 0;
        } else {
            cpu_array[i].wake_on_write = HHDM_CAST(uint64_t *, cpu->wake_on_write);
        }
        cpu_array[i].lapic_id = cpu->lapic_id;
    }
    boot_info->cpu_count = cpu_count;
    boot_info->cpus = HHDM_CAST(tartarus_cpu_t *, cpu_array);
    boot_info->bsp_index = bsp_index;
#endif

    boot_info->module_count = 0;
    boot_info->modules = HHDM_CAST(tartarus_module_t *, NULL);

    void *stack = pmm_alloc(PMM_AREA_STANDARD, 16) + 16 * PMM_PAGE_SIZE;
    protocol_tartarus_handoff(kernel->entry, stack, HHDM_CAST(uint64_t, boot_info));
    __builtin_unreachable();
}