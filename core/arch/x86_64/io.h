#pragma once
#include <stdint.h>

static inline void arch_io_port_write_u8(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void arch_io_port_write_u16(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void arch_io_port_write_u32(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t arch_io_port_read_u8(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t arch_io_port_read_u16(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t arch_io_port_read_u32(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void arch_io_mem_write_u8(uintptr_t addr, uint8_t value) {
    asm volatile("mfence\nmovb %1, (%0)" : : "r"(addr), "q"(value) : "memory");
}

static inline void arch_io_mem_write_u16(uintptr_t addr, uint16_t value) {
    asm volatile("mfence\nmovw %1, (%0)" : : "r"(addr), "q"(value) : "memory");
}

static inline void arch_io_mem_write_u32(uintptr_t addr, uint32_t value) {
    asm volatile("mfence\nmovl %1, (%0)" : : "r"(addr), "q"(value) : "memory");
}

[[nodiscard]] static inline uint8_t arch_io_mem_read_u8(uintptr_t addr) {
    uint8_t ret;
    asm volatile("mfence\nmovb (%1), %0" : "=q"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint16_t arch_io_mem_read_u16(uintptr_t addr) {
    uint16_t ret;
    asm volatile("mfence\nmovw (%1), %0" : "=q"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint32_t arch_io_mem_read_u32(uintptr_t addr) {
    uint32_t ret;
    asm volatile("mfence\nmovl (%1), %0" : "=q"(ret) : "r"(addr) : "memory");
    return ret;
}
