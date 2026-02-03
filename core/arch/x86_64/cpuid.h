#pragma once

#include <stdint.h>

typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} x86_64_cpuid_registers_t;

static inline x86_64_cpuid_registers_t x86_64_cpuid(uint32_t leaf) {
    x86_64_cpuid_registers_t registers;
    asm volatile("cpuid" : "=a"(registers.eax), "=b"(registers.ebx), "=c"(registers.ecx), "=d"(registers.edx) : "a"(leaf), "c"(0));
    return registers;
}
