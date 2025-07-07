#pragma once

#include "dev/acpi.h"

#include <stdint.h>

typedef struct x86_64_smp_cpu {
    uint8_t acpi_id;
    uint8_t lapic_id;
    bool is_bsp;
    bool init_failed;
    uint64_t *wake_on_write;
    struct x86_64_smp_cpu *next;
} x86_64_smp_cpu_t;

x86_64_smp_cpu_t *x86_64_smp_initialize_aps(acpi_sdt_header_t *madt_header, void *reserved_init_page, void *address_space, uint64_t stack_pgcnt, uint64_t stack_offset);
