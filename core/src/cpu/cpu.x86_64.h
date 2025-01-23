#pragma once

#include <stdint.h>

#define CPU_MSR_EFER 0xC0000080

extern bool g_cpu_nx_support;

static inline uint64_t cpu_msr_read(uint64_t msr) {
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"((uint32_t) msr));
    return low + ((uint64_t) high << 32);
}

static inline void cpu_msr_write(uint64_t msr, uint64_t value) {
    asm volatile("wrmsr" : : "a"((uint32_t) value), "d"((uint32_t) (value >> 32)), "c"((uint32_t) msr));
}

void cpu_enable_nx();