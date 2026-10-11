#include "arch/smp.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi.h"
#include "dev/acpi/tables/fadt.h"
#include "dev/acpi/tables/madt.h"
#include "dev/firmware.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include "arch/aarch64/cpu.h"

#include <stddef.h>
#include <stdint.h>

#define PSCI_COMPLIANT (1 << 0)
#define PSCI_USE_HVC (1 << 1)

#define GICC_FLAG_ENABLED (1 << 0)

#define CYCLES_100K 100000

// has to match boot_info in apinit.S
typedef struct [[gnu::packed]] {
    uint64_t init;
    uint64_t txsz;
    uint64_t ttbr0;
    uint64_t ttbr1;
    uint64_t hhdm_offset;
    uint64_t stack;
    uint64_t park_address;
} ap_info_t;

extern nullptr_t g_apinit_start[];
extern nullptr_t g_apinit_end[];

extern int64_t apinit_smc(uint64_t mpidr, uint64_t entry, uint64_t context);
extern int64_t apinit_hvc(uint64_t mpidr, uint64_t entry, uint64_t context);

void smp_initialize_aps(smp_cpu_t *cpu_list, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset) {
    fadt_t *fadt = (fadt_t *) acpi_find_table(firmware_get_rdsp(), "FACP");
    if(fadt == nullptr) panic("ACPI FADT table not present");

    if(fadt->sdt_header.length < sizeof(fadt_t) || (fadt->arm_boot_arch & PSCI_COMPLIANT) == 0) panic("No supported method to bring up APs");

    bool use_hvc = (fadt->arm_boot_arch & PSCI_USE_HVC) != 0;

    void *apinit_page = pmm_alloc(PMM_AREA_STANDARD, 1);

    size_t apinit_size = (uintptr_t) g_apinit_end - (uintptr_t) g_apinit_start;
    if(apinit_size + sizeof(ap_info_t) > PMM_GRANULARITY) panic("Unable to fit AP initialization code into a page");
    memcpy(apinit_page, (void *) g_apinit_start, apinit_size);

    ap_info_t *ap_info = (ap_info_t *) (apinit_page + apinit_size);


    for(smp_cpu_t *cpu = cpu_list; cpu; cpu = cpu->next) {
        if(cpu->is_bsp) continue;
        log(LOG_LEVEL_INFO, "Initializing cpu [mpidr %llu]", cpu->mpidr);

        cpu->park_address = heap_alloc(sizeof(uint64_t) + sizeof(uint64_t));
        *cpu->park_address = 0;


        ap_info->init = 0;
        ap_info->txsz = 64 - PTM_VA_BITS(address_space);
        ap_info->hhdm_offset = hhdm_offset;
        ap_info->ttbr0 = (uintptr_t) address_space->top_page_tables[0];
        ap_info->ttbr1 = (uintptr_t) address_space->top_page_tables[1];
        ap_info->stack = (uintptr_t) pmm_alloc(PMM_AREA_STANDARD, stack_pgcnt) + (PMM_GRANULARITY * stack_pgcnt) + hhdm_offset;
        ap_info->park_address = (uintptr_t) cpu->park_address;

        aarch64_cpu_dcache_clean_poc_range((uintptr_t) apinit_page, PMM_GRANULARITY);
        aarch64_cpu_icache_sync_pou_range((uintptr_t) apinit_page, PMM_GRANULARITY);

        int64_t retval;
        if(use_hvc) {
            retval = apinit_hvc(cpu->mpidr, (uintptr_t) apinit_page, 0);
        } else {
            retval = apinit_smc(cpu->mpidr, (uintptr_t) apinit_page, 0);
        }

        if(retval == 0) {
            for(int i = 0; i < 100000; i++) {
                aarch64_cpu_counter_block(CYCLES_100K);

                uint64_t value = 0;
                asm volatile("ldar %0, %1" : "=r"(value) : "m"(ap_info->init) : "memory");
                if(value == 67) goto end;
            }
            log(LOG_LEVEL_WARN, "AP timed out");
            cpu->init_failed = true;
            goto end;
        } else {
            cpu->init_failed = true;
            switch(retval) {
                case -1: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: not supported"); break;
                case -2: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: invalid param(s)"); break;
                case -3: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: denied"); break;
                case -4: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: already on"); break;
                case -5: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: already pending"); break;
                case -6: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: internal failure"); break;
                case -7: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: not present"); break;
                case -8: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: disabled"); break;
                case -9: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: invalid address"); break;
                default: log(LOG_LEVEL_WARN, "PSCI_CPU_ON: unknown error code %lli", retval); break;
            }
        }

    end:
        if(cpu->init_failed) {
            log(LOG_LEVEL_WARN, "Failed to initialize cpu [mpidr %u]", cpu->mpidr);
        } else {
            log(LOG_LEVEL_INFO, "Successfully initialized cpu [mpidr %u]", cpu->mpidr);
        }
    }
}
