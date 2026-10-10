#pragma once
#include <stdint.h>

#define ARCH_CSR_READ(csr)                                     \
    ({                                                         \
        uint64_t _val;                                         \
        asm volatile("csrr %0, " #csr : "=r"(_val)::"memory"); \
        _val;                                                  \
    })

#define ARCH_CSR_WRITE(csr, val)                                 \
    do {                                                         \
        uint64_t _v = (uint64_t) (val);                          \
        asm volatile("csrw " #csr ", %0" ::"rK"(_v) : "memory"); \
    } while(0)


#define ARCH_CSR_SATP_MODE_SV39 ((uint64_t) (8ULL << 60))
#define ARCH_CSR_SATP_MODE_SV48 ((uint64_t) (9ULL << 60))
#define ARCH_CSR_SATP_MODE_SV57 ((uint64_t) (10ULL << 60))

#define ARCH_CSR_SATP_PPN_MASK ((uint64_t) 0x00000fffffffffffull)

#define ARCH_CSR_SATP_MAKE(mode, root_pa) ((mode) | (((root_pa) >> 12) & ARCH_CSR_SATP_PPN_MASK))
