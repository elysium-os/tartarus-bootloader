#pragma once

#include "arch/ptm.h"

#include <stdint.h>

typedef struct smp_cpu {
    uint32_t acpi_id;
#ifdef __ARCH_X86_64
    uint8_t lapic_id;
#elif __ARCH_AARCH64
    uint64_t mpidr;
#endif

    bool is_bsp;
    bool init_failed;

    uint64_t *park_address;

    struct smp_cpu *next;
} smp_cpu_t;

#ifdef __ARCH_X86_64
extern void *g_smp_reserved_init_page;
#endif

smp_cpu_t *smp_initialize_aps(void *rsdp, ptm_address_space_t *address_space, uint64_t stack_pgcnt, uint64_t hhdm_offset);
