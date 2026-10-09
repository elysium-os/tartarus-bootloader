#include <stdint.h>

static inline void arch_io_mem_write_u8(uintptr_t addr, uint8_t value) {
    asm volatile("fence iorw, ow\n\tsb %0, 0(%1)" : : "r"(value), "r"(addr) : "memory");
}

static inline void arch_io_mem_write_u16(uintptr_t addr, uint16_t value) {
    asm volatile("fence iorw, ow\n\tsh %0, 0(%1)" : : "r"(value), "r"(addr) : "memory");
}

static inline void arch_io_mem_write_u32(uintptr_t addr, uint32_t value) {
    asm volatile("fence iorw, ow\n\tsw %0, 0(%1)" : : "r"(value), "r"(addr) : "memory");
}

static inline void arch_io_mem_write_u64(uintptr_t addr, uint64_t value) {
    asm volatile("fence iorw, ow\n\tsd %0, 0(%1)" : : "r"(value), "r"(addr) : "memory");
}

[[nodiscard]] static inline uint8_t arch_io_mem_read_u8(uintptr_t addr) {
    uint8_t ret;
    asm volatile("lbu %0, 0(%1)\n\tfence ir, iorw" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint16_t arch_io_mem_read_u16(uintptr_t addr) {
    uint16_t ret;
    asm volatile("lhu %0, 0(%1)\n\tfence ir, iorw" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint32_t arch_io_mem_read_u32(uintptr_t addr) {
    uint32_t ret;
    asm volatile("lw %0, 0(%1)\n\tfence ir, iorw" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}

[[nodiscard]] static inline uint64_t arch_io_mem_read_u64(uintptr_t addr) {
    uint64_t ret;
    asm volatile("ld %0, 0(%1)\n\tfence ir, iorw" : "=r"(ret) : "r"(addr) : "memory");
    return ret;
}
