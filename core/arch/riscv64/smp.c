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

void smp_initialize_aps(smp_cpu_t *cpu_list, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset) {
    uint64_t mode = 0;
    switch(address_space->level_count) {
        case 3: mode = ARCH_CSR_SATP_MODE_SV39; break;
        case 4: mode = ARCH_CSR_SATP_MODE_SV48; break;
        case 5: mode = ARCH_CSR_SATP_MODE_SV57; break;
    }
    uint64_t satp = ARCH_CSR_SATP_MAKE(mode, (uintptr_t) address_space->top_page_table);

    for(smp_cpu_t *cpu = cpu_list; cpu; cpu = cpu->next) {
        if(cpu->is_bsp) continue;
        log(LOG_LEVEL_INFO, "Initializing cpu [hartid %" LOGFMT_U64 "]", cpu->hartid);

        cpu->park_address = heap_alloc(sizeof(uint64_t) + sizeof(uint64_t));
        *cpu->park_address = 0;

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
                if(__atomic_load_n(&ap_info->init, __ATOMIC_ACQUIRE) == 67) goto end;
            }
            log(LOG_LEVEL_WARN, "AP timed out");
            cpu->init_failed = true;
        }

    end:
        if(cpu->init_failed) {
            log(LOG_LEVEL_WARN, "Failed to initialize cpu [hartid %" LOGFMT_U64 "]", cpu->hartid);
        } else {
            log(LOG_LEVEL_INFO, "Successfully initialized cpu [hartid %" LOGFMT_U64 "]", cpu->hartid);
        }
    }
}
