#include "arch/smp.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi/tables/madt.h"
#include "dev/firmware.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/cpuid.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/lapic.h"
#include "arch/x86_64/tsc.h"

#include <stdint.h>

#define CYCLES_10MIL 10000000

// has to match boot_info in apinit.asm
typedef struct [[gnu::packed]] {
    uint8_t init;
    uint8_t lapic_id;
    uint32_t pml4;
    uint64_t park_address;
    uint64_t stack;
    uint16_t gdtr_limit;
    uint32_t gdtr_base;
    uint8_t set_nx;
    uint8_t set_la57;
} ap_info_t;

extern nullptr_t g_apinit_start[];
extern nullptr_t g_apinit_end[];

void *g_smp_reserved_init_page;

void smp_initialize_aps(smp_cpu_t *cpu_list, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset) {
    size_t apinit_size = (uintptr_t) g_apinit_end - (uintptr_t) g_apinit_start;
    if(apinit_size + sizeof(ap_info_t) > PMM_GRANULARITY) panic("Unable to fit AP initialization code into a page");
    memcpy(g_smp_reserved_init_page, (void *) g_apinit_start, apinit_size);

    ap_info_t *ap_info = (ap_info_t *) (g_smp_reserved_init_page + apinit_size);
    ap_info->pml4 = (uintptr_t) address_space->top_page_table;
    ap_info->gdtr_limit = g_x86_64_gdt_limit;
    ap_info->gdtr_base = (uintptr_t) g_x86_64_gdt;
    ap_info->set_nx = g_x86_64_cpu_nx_support;
    ap_info->set_la57 = g_x86_64_cpu_la57_support;

    for(smp_cpu_t *cpu = cpu_list; cpu; cpu = cpu->next) {
        if(cpu->is_bsp) continue;

        log(LOG_LEVEL_INFO, "Initializing cpu [lapic id %u]", cpu_list->lapic_id);
        cpu->park_address = heap_alloc(sizeof(uint64_t) + sizeof(uint64_t));
        *cpu->park_address = 0;

        ap_info->init = 0;
        ap_info->lapic_id = cpu->lapic_id;
        ap_info->park_address = (uintptr_t) cpu->park_address;
        ap_info->stack = (uintptr_t) pmm_alloc(PMM_AREA_STANDARD, stack_pgcnt) + (PMM_GRANULARITY * stack_pgcnt) + hhdm_offset;

        asm volatile("" : : : "memory");

        x86_64_lapic_ipi_init(cpu->lapic_id);
        x86_64_tsc_block(CYCLES_10MIL);
        x86_64_lapic_ipi_startup(cpu->lapic_id, g_smp_reserved_init_page);

        if(!cpu->init_failed) {
            for(int i = 0; i < 1000; i++) {
                x86_64_tsc_block(CYCLES_10MIL);
                uint8_t value = 0;
                asm volatile("lock xadd %0, %1" : "+r"(value) : "m"(ap_info->init) : "memory");
                if(value > 0) goto end;
            }
            log(LOG_LEVEL_WARN, "AP timed out");
            cpu->init_failed = true;
        }

    end:
        if(cpu->init_failed) {
            log(LOG_LEVEL_WARN, "Failed to initialize cpu [lapic id %u]", cpu->lapic_id);
        } else {
            log(LOG_LEVEL_INFO, "Successfully initialized cpu [lapic id %u]", cpu->lapic_id);
        }
    }
}
