#include "arch/smp.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi/tables/madt.h"
#include "dev/firmware.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "smoldtb.h"

#if defined(__ARCH_X86_64)
#define LAPIC_FLAG_ENABLED (1 << 0)

static inline smp_cpu_t *parse_lapic_entry(madt_record_t *record) {
    if(record->length < sizeof(madt_record_lapic_t)) {
        log(LOG_LEVEL_WARN, "Found MADT LAPIC record that is shorter than expected (%#x/%#zx)", record->length, sizeof(madt_record_lapic_t));
        return nullptr;
    }

    madt_record_lapic_t *lapic_record = (madt_record_lapic_t *) record;
    if((lapic_record->flags & LAPIC_FLAG_ENABLED) == 0) return nullptr;

    smp_cpu_t *cpu = heap_alloc(sizeof(smp_cpu_t));
    cpu->init_failed = false;
    cpu->park_address = nullptr;
    cpu->lapic_id = lapic_record->lapic_id;
    cpu->is_bsp = lapic_record->lapic_id == firmware_get()->boot_cpu_id;
    return cpu;
}
#endif

#if defined(__ARCH_AARCH64)
#define GICC_FLAG_ENABLED (1 << 0)

static inline smp_cpu_t *parse_gicc_entry(madt_record_t *record) {
    if(record->length < sizeof(madt_record_gicc_t)) {
        log(LOG_LEVEL_WARN, "Found MADT GICC record that is shorter than expected (%#x/%#zx)", record->length, sizeof(madt_record_gicc_t));
        return nullptr;
    }

    madt_record_gicc_t *gicc_record = (madt_record_gicc_t *) record;
    if((gicc_record->flags & GICC_FLAG_ENABLED) == 0) return nullptr;

    smp_cpu_t *cpu = heap_alloc(sizeof(smp_cpu_t));
    cpu->init_failed = false;
    cpu->park_address = nullptr;
    cpu->mpidr = gicc_record->mpidr;
    cpu->is_bsp = gicc_record->mpidr == firmware_get()->boot_cpu_id;
    return cpu;
}
#endif

#if defined(__ARCH_RISCV64)
#define RINTC_FLAG_ENABLED (1 << 0)

static inline smp_cpu_t *parse_rintc_entry(madt_record_t *record) {
    if(record->length < sizeof(madt_record_rintc_t)) {
        log(LOG_LEVEL_WARN, "Found MADT RINTC record that is shorter than expected (%#x/%#zx)", record->length, sizeof(madt_record_rintc_t));
        return nullptr;
    }

    madt_record_rintc_t *rintc_record = (madt_record_rintc_t *) record;
    if((rintc_record->flags & RINTC_FLAG_ENABLED) == 0) return nullptr;

    smp_cpu_t *cpu = heap_alloc(sizeof(smp_cpu_t));
    cpu->init_failed = false;
    cpu->park_address = nullptr;
    cpu->hartid = rintc_record->hartid;
    cpu->is_bsp = rintc_record->hartid == firmware_get()->boot_cpu_id;
    return cpu;
}
#endif


smp_cpu_t *smp_discover_cpus_acpi() {
    madt_t *madt = (madt_t *) acpi_find_table(firmware_get_rdsp(), "APIC");
    if(madt == nullptr) {
        log(LOG_LEVEL_WARN, "Failed to find APIC table");
        return nullptr;
    }

    smp_cpu_t *cpus = nullptr;
    for(size_t count = sizeof(madt_t); count < madt->sdt_header.length; count += ((madt_record_t *) ((uintptr_t) madt + count))->length) {
        madt_record_t *record = (madt_record_t *) ((uintptr_t) madt + count);
        smp_cpu_t *cpu = nullptr;
        switch(record->type) {
#if defined(__ARCH_X86_64)
            case MADT_LAPIC:   cpu = parse_lapic_entry(record); break;
            case MADT_LX2APIC: log(LOG_LEVEL_WARN, "x2APIC not currently supported, ignoring MADT entry"); break;
#endif
#if defined(__ARCH_AARCH64)
            case MADT_GICC: cpu = parse_gicc_entry(record); break;
#endif
#if defined(__ARCH_RISCV64)
            case MADT_RINTC: cpu = parse_rintc_entry(record); break;
#endif
        }

        if(cpu != nullptr) {
            cpu->next = cpus;
            cpus = cpu;
        }
    }
    return cpus;
}

smp_cpu_t *smp_discover_cpus_device_tree() {
#if defined(__ARCH_RISCV64)
    smp_cpu_t *cpus = nullptr;
    for(dtb_node *node = dtb_find_compatible(nullptr, "riscv"); node != nullptr; node = dtb_find_compatible(node, "riscv")) {
        dtb_prop *device_type = dtb_find_prop(node, "device_type");
        if(!device_type) continue;

        const char *type = dtb_read_prop_string(device_type, 0);
        if(!string_eq(type, "cpu")) continue;

        dtb_prop *status_prop = dtb_find_prop(node, "status");
        if(!status_prop) continue;

        const char *status = dtb_read_prop_string(status_prop, 0);
        if(!string_eq(status, "okay") && !string_eq(status, "ok")) { continue; }

        dtb_prop *isa_base_prop = dtb_find_prop(node, "riscv,isa-base");
        if(!isa_base_prop) continue;

        const char *isa_base = dtb_read_prop_string(isa_base_prop, 0);
        if(!string_eq(isa_base, "rv64i")) {
            log(LOG_LEVEL_WARN, "Rejecting non rv64i cpu");
            continue;
        }

        dtb_prop *hart_id_prop = dtb_find_prop(node, "reg");
        if(!hart_id_prop) { continue; }
        uint64_t hart_id;
        dtb_read_prop_1(hart_id_prop, 1, &hart_id);

        smp_cpu_t *cpu = heap_alloc(sizeof(smp_cpu_t));
        cpu->next = cpus;
        cpus = cpu;

        cpu->init_failed = false;
        cpu->park_address = nullptr;
        cpu->hartid = hart_id;
        if(cpu->hartid == firmware_get()->boot_cpu_id) {
            cpu->is_bsp = true;
            continue;
        }
    }

    return cpus;
#else
    log(LOG_LEVEL_WARN, "Device tree support for this platform is unimplemented");
    return nullptr;
#endif
}

smp_cpu_t *smp_discover_cpus() {
    smp_cpu_t *result = nullptr;

    result = smp_discover_cpus_device_tree();
    if(result != nullptr) return result;

    result = smp_discover_cpus_acpi();
    if(result != nullptr) return result;

    log(LOG_LEVEL_WARN, "Failed to detect smp cpu information, will not start APs!");

    return nullptr;
}
