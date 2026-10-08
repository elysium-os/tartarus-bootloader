#pragma once

#include <stddef.h>
#include <stdint.h>

#define AARCH64_CPU_READ_SYSTEM_REG(REGISTER)           \
    ({                                                  \
        uint64_t reg;                                   \
        asm volatile("mrs %0, " #REGISTER : "=r"(reg)); \
        reg;                                            \
    })

#define AARCH64_CPU_COUNTER_READ() AARCH64_CPU_READ_SYSTEM_REG(cntpct_el0)

#define AARCH64_CPU_DCACHE_LINE_SIZE (((AARCH64_CPU_READ_SYSTEM_REG(ctr_el0) >> 16) & 0xF) << 4)
#define AARCH64_CPU_ICACHE_LINE_SIZE ((AARCH64_CPU_READ_SYSTEM_REG(ctr_el0) & 0xF) << 4)

static inline void aarch64_cpu_dcache_clean_poc_range(uintptr_t address, size_t size) {
    if(size == 0) return;

    size_t line_size = AARCH64_CPU_DCACHE_LINE_SIZE;

    uintptr_t end = address + size;
    for(uintptr_t curr = address & ~(line_size - 1); curr < end; curr += line_size) asm volatile("dc cvac, %0" : : "r"(curr) : "memory");

    asm volatile("dsb sy");
    asm volatile("isb");
}

static inline void aarch64_cpu_icache_sync_pou_range(uintptr_t address, size_t size) {
    if(size == 0) return;

    size_t line_size = AARCH64_CPU_ICACHE_LINE_SIZE;

    uintptr_t end = address + size;
    for(uintptr_t curr = address & ~(line_size - 1); curr < end; curr += line_size) asm volatile("ic ivau, %0" : : "r"(curr) : "memory");

    asm volatile("dsb sy");
    asm volatile("isb");
}

static inline void aarch64_cpu_counter_block(size_t cycles) {
    uint64_t target = AARCH64_CPU_COUNTER_READ() + cycles;
    while(AARCH64_CPU_COUNTER_READ() < target);
}
