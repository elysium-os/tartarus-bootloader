#pragma once
#include <stdint.h>

static inline void arch_io_mem_write_u8(uintptr_t addr, uint8_t value) {
    asm volatile("dmb sy\n\tstrb %w0, [%1]" : : "r"(value), "r"(addr) : "memory");
}

static inline void arch_io_mem_write_u16(uintptr_t addr, uint16_t value) {
    asm volatile("dmb sy\n\tstrh %w0, [%1]" : : "r"(value), "r"(addr) : "memory");
}

static inline void arch_io_mem_write_u32(uintptr_t addr, uint32_t value) {
    asm volatile("dmb sy\n\tstr %w0, [%1]" : : "r"(value), "r"(addr) : "memory");
}

[[nodiscard]] static inline uint8_t arch_io_mem_read_u8(uintptr_t addr) {
    uint8_t ret;
    asm volatile("ldrb %w0, [%1]\n\tdmb sy" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint16_t arch_io_mem_read_u16(uintptr_t addr) {
    uint16_t ret;
    asm volatile("ldrh %w0, [%1]\n\tdmb sy" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint32_t arch_io_mem_read_u32(uintptr_t addr) {
    uint32_t ret;
    asm volatile("ldr %w0, [%1]\n\tdmb sy" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}
