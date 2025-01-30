#include "cpu.h"

#include "common/log.h"

#include "arch/x86_64/msr.h"

bool g_x86_64_cpu_nx_support = false;
bool g_x86_64_cpu_pdpe1gb_support = false;
bool g_x86_64_cpu_lapic_support = false;

void x86_64_cpu_init() {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001), "c"(0));
    g_x86_64_cpu_nx_support = (edx & (1 << 20)) != 0;
    g_x86_64_cpu_pdpe1gb_support = (edx & (1 << 26)) != 0;

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x1), "c"(0));
    g_x86_64_cpu_lapic_support = (edx & (1 << 9)) != 0;

    if(g_x86_64_cpu_nx_support) {
        x86_64_msr_write(X86_64_MSR_EFER, x86_64_msr_read(X86_64_MSR_EFER) | (1 << 11));
    } else {
        log(LOG_LEVEL_WARN, "no support for EFER.NXE");
    }
    if(!g_x86_64_cpu_pdpe1gb_support) log(LOG_LEVEL_WARN, "no support for 1gb mappings");
}

void arch_cpu_halt() {
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}
