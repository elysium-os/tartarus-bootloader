#pragma once

#include <stddef.h>
#include <stdint.h>

#define RISCV_CPU_READ_CYCLE()                    \
    ({                                            \
        uintptr_t value;                          \
        asm volatile("rdcycle %0" : "=r"(value)); \
        value;                                    \
    })

static inline void riscv_cpu_counter_block(size_t cycles) {
    uint64_t target = RISCV_CPU_READ_CYCLE() + cycles;
    while(RISCV_CPU_READ_CYCLE() < target);
}
