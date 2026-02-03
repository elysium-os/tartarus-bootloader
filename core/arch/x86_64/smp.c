#include "smp.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include "arch/x86_64/cpu.h"
#include "arch/x86_64/cpuid.h"
#include "arch/x86_64/gdt.h"
#include "arch/x86_64/lapic.h"
#include "arch/x86_64/tsc.h"

#define CYCLES_10MIL 10000000

typedef struct [[gnu::packed]] {
    acpi_sdt_header_t sdt_header;
    uint32_t local_apic_address;
    uint32_t flags;
} madt_t;

typedef enum {
    MADT_LAPIC = 0,
    MADT_IOAPIC,
    MADT_SOURCE_OVERRIDE,
    MADT_NMI_SOURCE,
    MADT_NMI,
    MADT_LAPIC_ADDRESS,
    MADT_LX2APIC = 9
} madt_record_types_t;

typedef struct [[gnu::packed]] {
    uint8_t type;
    uint8_t length;
} madt_record_t;

typedef struct [[gnu::packed]] {
    madt_record_t base;
    uint8_t acpi_processor_id;
    uint8_t lapic_id;
    uint32_t flags;
} madt_record_lapic_t;

// has to match boot_info in apinit.asm
typedef struct [[gnu::packed]] {
    uint8_t init;
    uint8_t lapic_id;
    uint32_t pml4;
    uint64_t wait_on_address;
    uint64_t stack;
    uint16_t gdtr_limit;
    uint32_t gdtr_base;
    uint8_t set_nx;
} ap_info_t;

extern nullptr_t g_apinit_start[];
extern nullptr_t g_apinit_end[];


x86_64_smp_cpu_t *x86_64_smp_initialize_aps(acpi_sdt_header_t *madt_header, void *reserved_init_page, void *address_space, uint64_t stack_pgcnt, uint64_t stack_offset) {
    madt_t *madt = (madt_t *) madt_header;
    uint8_t bsp_id = x86_64_cpuid(1).ebx >> 24;
    log(LOG_LEVEL_INFO, "BSP ID: %u", bsp_id);
    if(bsp_id != x86_64_lapic_id()) panic("Current lapic id does not match BSP id");

    size_t apinit_size = (uintptr_t) g_apinit_end - (uintptr_t) g_apinit_start;
    if(apinit_size + sizeof(ap_info_t) > PMM_GRANULARITY) panic("unable to fit AP initialization code into a page");
    memcpy(reserved_init_page, (void *) g_apinit_start, apinit_size);

    ap_info_t *ap_info = (ap_info_t *) (reserved_init_page + apinit_size);
    ap_info->pml4 = (uintptr_t) address_space;
    ap_info->gdtr_limit = g_x86_64_gdt_limit;
    ap_info->gdtr_base = (uintptr_t) g_x86_64_gdt;
    ap_info->set_nx = g_x86_64_cpu_nx_support;

    x86_64_smp_cpu_t *cpus = NULL;
    for(size_t count = sizeof(madt_t); count < madt->sdt_header.length; count += ((madt_record_t *) ((uintptr_t) madt + count))->length) {
        madt_record_t *record = (madt_record_t *) ((uintptr_t) madt + count);
        switch(record->type) {
            case MADT_LAPIC:
                madt_record_lapic_t *lapic_record = (madt_record_lapic_t *) record;

                x86_64_smp_cpu_t *cpu = heap_alloc(sizeof(x86_64_smp_cpu_t));
                cpu->next = cpus;
                cpus = cpu;

                cpu->init_failed = false;
                cpu->acpi_id = lapic_record->acpi_processor_id;
                cpu->lapic_id = lapic_record->lapic_id;
                cpu->wake_on_write = NULL;
                cpu->is_bsp = false;
                if(lapic_record->lapic_id == bsp_id) {
                    cpu->is_bsp = true;
                    goto success;
                }
                cpu->wake_on_write = heap_alloc(sizeof(uint64_t));
                *cpu->wake_on_write = 0;

                ap_info->init = 0;
                ap_info->lapic_id = lapic_record->lapic_id;
                ap_info->wait_on_address = (uintptr_t) cpu->wake_on_write;
                ap_info->stack = (uintptr_t) pmm_alloc(PMM_AREA_STANDARD, stack_pgcnt) + (PMM_GRANULARITY * stack_pgcnt) + stack_offset;

                x86_64_lapic_ipi_init(lapic_record->lapic_id);
                x86_64_tsc_block(CYCLES_10MIL);
                x86_64_lapic_ipi_startup(lapic_record->lapic_id, reserved_init_page);

                for(int i = 0; i < 1000; i++) {
                    x86_64_tsc_block(CYCLES_10MIL);
                    uint8_t value = 0;
                    asm volatile("lock xadd %0, %1" : "+r"(value) : "m"(ap_info->init) : "memory");
                    if(value > 0) goto success;
                }

                log(LOG_LEVEL_WARN, "CPU(lapic id %u) failed to initialize", lapic_record->lapic_id);
                cpu->init_failed = true;
                break;

            success:
                log(LOG_LEVEL_INFO, "CPU(lapic id %u) initialized", lapic_record->lapic_id);
                break;
            case MADT_LX2APIC: log(LOG_LEVEL_WARN, "x2APIC not currently supported, ignoring MADT entry"); break;
        }
    }
    return cpus;
}
