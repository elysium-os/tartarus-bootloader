#include "cpu.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/firmware.h"

#include "arch/x86_64/cpuid.h"
#include "arch/x86_64/lapic.h"
#include "arch/x86_64/msr.h"

bool g_x86_64_cpu_nx_support = false;
bool g_x86_64_cpu_pdpe1gb_support = false;
bool g_x86_64_cpu_lapic_support = false;
bool g_x86_64_cpu_la57_support = false;

void arch_cpu_init() {
    x86_64_cpuid_registers_t regs;

    regs = x86_64_cpuid(0x80000001);
    g_x86_64_cpu_nx_support = (regs.edx & (1 << 20)) != 0;
    g_x86_64_cpu_pdpe1gb_support = (regs.edx & (1 << 26)) != 0;

    regs = x86_64_cpuid(1);
    g_x86_64_cpu_lapic_support = (regs.edx & (1 << 9)) != 0;

    regs = x86_64_cpuid(7);
    g_x86_64_cpu_la57_support = (regs.ecx & (1 << 16)) != 0;

    if(g_x86_64_cpu_nx_support) {
        x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | (1 << 11));
    } else {
        log(LOG_LEVEL_WARN, "no support for EFER.NXE");
    }
    if(!g_x86_64_cpu_pdpe1gb_support) log(LOG_LEVEL_WARN, "no support for 1gb mappings");

    uint8_t bsp_id = x86_64_cpuid(1).ebx >> 24;
    log(LOG_LEVEL_INFO, "BSP ID: %u", bsp_id);
    if(bsp_id != x86_64_lapic_id()) panic("Current lapic id does not match BSP id");

    firmware_get()->boot_cpu_id = bsp_id;
}

[[noreturn]] void arch_cpu_halt() {
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}
