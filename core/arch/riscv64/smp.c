#include "arch/smp.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi.h"
#include "dev/acpi/tables/fadt.h"
#include "dev/acpi/tables/madt.h"
#include "dev/firmware.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "smoldtb.h"

#include "arch/riscv64/cpu.h"
#include "arch/riscv64/csr.h"
#include "arch/riscv64/sbi.h"

#include <stddef.h>
#include <stdint.h>

#define CYCLES_100K 100000

typedef struct {
    uint64_t init;
    uint64_t stack;
    uint64_t satp;
    uint64_t park_address;
} ap_info_t;

extern nullptr_t g_apinit_start[];


static smp_cpu_t *discover_aps() {
    // @todo: acpi support
    dtb_node *cpu_node = dtb_find("/cpus");
    if(cpu_node == nullptr) {
        panic("fdt: /cpus node not found\n");
        return nullptr;
    }

    smp_cpu_t *cpus = nullptr;
    for(dtb_node *node = dtb_get_child(cpu_node); node; node = dtb_get_sibling(node)) {
        dtb_prop *device_type = dtb_find_prop(node, "device_type");
        if(!device_type) continue;

        const char *type = dtb_read_prop_string(device_type, 0);
        if(!string_eq(type, "cpu")) continue;

        dtb_prop *status_prop = dtb_find_prop(node, "status");
        if(!status_prop) continue;

        const char *status = dtb_read_prop_string(status_prop, 0);
        if(!string_eq(status, "okay") && !string_eq(status, "ok")) { continue; }

        dtb_prop *hart_id_prop = dtb_find_prop(node, "reg");
        if(!hart_id_prop) { continue; }
        uint64_t hart_id;
        dtb_read_prop_1(hart_id_prop, 1, &hart_id);


        smp_cpu_t *cpu = heap_alloc(sizeof(smp_cpu_t));
        cpu->next = cpus;
        cpus = cpu;

        cpu->init_failed = false;

        // @todo: what do we do on non-acpi platforms
        cpu->acpi_id = 0;
        cpu->hartid = hart_id;

        cpu->park_address = nullptr;
        cpu->is_bsp = false;
        if(cpu->hartid == firmware_get()->boot_cpu_id) {
            cpu->is_bsp = true;
            continue;
        }

        cpu->park_address = heap_alloc(sizeof(uint64_t) + sizeof(uint64_t));
        *cpu->park_address = 0;
        *(cpu->park_address + 8) = 0;
    }

    // @todo: fall back to acpi if not found
    // panic("unimplemented...");
    return cpus;
}

smp_cpu_t *smp_initialize_aps(void *rsdp, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset) {
    (void) rsdp;

    uint64_t mode = 0;
    switch(address_space->level_count) {
        case 3: mode = ARCH_CSR_SATP_MODE_SV39; break;
        case 4: mode = ARCH_CSR_SATP_MODE_SV48; break;
        case 5: mode = ARCH_CSR_SATP_MODE_SV57; break;
    }
    uint64_t satp = ARCH_CSR_SATP_MAKE(mode, (uintptr_t) address_space->top_page_table);

    smp_cpu_t *cpu_list = discover_aps();
    for(smp_cpu_t *cpu = cpu_list; cpu; cpu = cpu->next) {
        if(cpu->is_bsp) continue;
        log(LOG_LEVEL_INFO, "Initializing cpu [hartid %llu]", cpu->hartid);

        ap_info_t *ap_info = (ap_info_t *) heap_alloc(sizeof(ap_info_t));
        ap_info->init = 0;
        ap_info->stack = (uintptr_t) pmm_alloc(PMM_AREA_STANDARD, stack_pgcnt) + (PMM_GRANULARITY * stack_pgcnt) + hhdm_offset;
        ap_info->satp = satp;
        ap_info->park_address = (uintptr_t) cpu->park_address;

        sbiret_t ret = sbi_start_hart(cpu->hartid, (uint64_t) g_apinit_start, (uint64_t) ap_info);
        cpu->init_failed = ret.error != 0;

        if(!cpu->init_failed) {
            for(int i = 0; i < 100000; i++) {
                riscv_cpu_counter_block(CYCLES_100K);

                if(__atomic_load_n(&ap_info->init, __ATOMIC_ACQUIRE) == 67) goto success;
            }
            log(LOG_LEVEL_WARN, "AP timed out");
            cpu->init_failed = true;
        }

    success:
        if(cpu->init_failed) {
            log(LOG_LEVEL_WARN, "Failed to initialize cpu [hartid %u]", cpu->hartid);
        } else {
            log(LOG_LEVEL_INFO, "Successfully initialized cpu [hartid %llu]", cpu->hartid);
        }
    }

    return cpu_list;
}
