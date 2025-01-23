#include "cpu.h"

#include "common/log.h"
#include "cpu/cpu.x86_64.h"

#include <cpuid.h>

bool g_cpu_nx_support = false;

void cpu_enable_nx() {
    unsigned int edx = 0, unused;
    if(__get_cpuid(0x80000001, &unused, &unused, &unused, &edx) != 0) g_cpu_nx_support = (edx & (1 << 20)) != 0;
    if(g_cpu_nx_support) {
        cpu_msr_write(CPU_MSR_EFER, cpu_msr_read(CPU_MSR_EFER) | (1 << 11));
    } else {
        log(LOG_LEVEL_WARN, "no support for EFER.NXE");
    }
}

void cpu_halt() {
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}
