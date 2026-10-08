#include "arch/cpu.h"

#include "common/panic.h"

#include "arch/aarch64/cpu.h"

#include <stdint.h>

void arch_cpu_init() {
    uint64_t aa64mmfr0 = AARCH64_CPU_READ_SYSTEM_REG(id_aa64mmfr0_el1);
    if(((aa64mmfr0 >> 28) & 0xF) == 0xF) panic("missing support for 4k page granularity");
}

[[noreturn]] void arch_cpu_halt() {
    for(;;) asm volatile("wfi");
    __builtin_unreachable();
}
