#include "arch/time.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/acpi.h"
#include "dev/acpi/tables/rhct.h"
#include "dev/firmware.h"
#include "smoldtb.h"

static uint64_t get_timebase_freq() {
    if(firmware_get_rdsp()) {
        rhct_t *rhct = (rhct_t *) acpi_find_table(firmware_get_rdsp(), "RHCT");
        if(rhct != nullptr) {
            log(LOG_LEVEL_WARN, "Failed to find RHCT table");
        } else {
            return rhct->time_base_frequency;
        }
    }

    dtb_node *cpus = dtb_find("/cpus");
    if(cpus == nullptr) {
        log(LOG_LEVEL_WARN, "/cpus node not found");
        panic("failed to detect timebase frequency");
    }

    dtb_prop *prop = dtb_find_prop(cpus, "timebase-frequency");
    if(prop == nullptr) {
        log(LOG_LEVEL_WARN, "timebase-frequency property not found in /cpus node");
        panic("failed to detect timebase frequency");
    }

    size_t value = 0;
    if(dtb_read_prop_1(prop, 1, &value) == 0) {
        log(LOG_LEVEL_WARN, "timebase-frequency property is empty");
        panic("failed to detect timebase frequency");
    }

    uint64_t frequency = value;
    log(LOG_LEVEL_INFO, "timebase-frequency = %llu Hz", (unsigned long long) frequency);
    return frequency;
}

static inline uint64_t get_rdtime(void) {
    uint64_t val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}

time_t arch_time() {
    static bool cached = false;
    static uint64_t freq = 0;

    if(!cached) {
        freq = get_timebase_freq();
        cached = true;
    }

    return get_rdtime() / freq;
}
