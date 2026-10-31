#pragma once

#include "arch/ptm.h"

#include <stdint.h>

typedef struct smp_cpu {
#ifdef __ARCH_X86_64
    uint64_t lapic_id;
#elif __ARCH_AARCH64
    uint64_t mpidr;
#elif __ARCH_RISCV64
    uint64_t hartid;
#endif

    bool is_bsp;
    bool init_failed;

    uint64_t *park_address;

    struct smp_cpu *next;
} smp_cpu_t;

#ifdef __ARCH_X86_64
extern void *g_smp_reserved_init_page;
#endif

smp_cpu_t *smp_discover_cpus();
void smp_initialize_aps(smp_cpu_t *cpu_list, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset);
