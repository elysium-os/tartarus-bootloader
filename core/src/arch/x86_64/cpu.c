#include "cpu.h"

#include "common/log.h"

#include "arch/x86_64/msr.h"

#include <cpuid.h>

bool g_x86_64_cpu_nx_support = false;

void x86_64_cpu_enable_nx() {
    unsigned int edx = 0, unused;
    if(__get_cpuid(0x80000001, &unused, &unused, &unused, &edx) != 0) g_x86_64_cpu_nx_support = (edx & (1 << 20)) != 0;
    if(g_x86_64_cpu_nx_support) {
        x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | (1 << 11));
    } else {
        log(LOG_LEVEL_WARN, "no support for EFER.NXE");
    }
}

void arch_cpu_halt() {
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}
