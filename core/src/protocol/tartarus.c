#include "protocol.h"
#include <stdint.h>
#include <common/log.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <drivers/acpi.h>

#define HHDM_MIN_SIZE 0x100000000
#define HHDM_OFFSET 0xFFFF800000000000
#define HHDM_FLAGS (VMM_FLAG_READ | VMM_FLAG_WRITE | VMM_FLAG_EXEC)

[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel [[maybe_unused]], fb_t fb [[maybe_unused]]) {
    // Setup HHDM
    void *address_space = vmm_create_address_space();
    vmm_map(address_space, PMM_PAGE_SIZE, PMM_PAGE_SIZE, HHDM_MIN_SIZE - PMM_PAGE_SIZE, HHDM_FLAGS);
    vmm_map(address_space, PMM_PAGE_SIZE, HHDM_OFFSET + PMM_PAGE_SIZE, HHDM_MIN_SIZE - PMM_PAGE_SIZE, HHDM_FLAGS);

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
        vmm_map(address_space, base, base, length, HHDM_FLAGS);
        vmm_map(address_space, base, HHDM_OFFSET + base, length, HHDM_FLAGS);
    }

    // ACPI
    acpi_rsdp_t *rsdp = acpi_find_rsdp();
    if(!rsdp) log_panic("PROTO_TARTARUS", "Could not locate RSDP");

    // Initialize SMP
    // if(config_read_bool(config, "SMP", true)) {
    //     if(smp_reserved_page == NULL) log_panic("CORE", "Unable to initialize SMP. No reserved SMP init page");
    //     if(!lapic_is_supported()) log_panic("CORE", "LAPIC not supported. LAPIC is required for SMP initialization");
    //     acpi_sdt_header_t *madt = acpi_find_table(rsdp, "APIC");
    //     if(!madt) log_panic("CORE", "ACPI MADT table not present");
    //     smp_cpu_t *cpus = smp_initialize_aps(madt, smp_reserved_page, address_space);
    //     log("CORE", "Initialized SMP");
    // }

    config_free(config);
    log_panic("PROTO_TARTARUS", "temporarily unimplemented");
}