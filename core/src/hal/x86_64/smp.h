#pragma once
#include <stdint.h>
#include <drivers/acpi.h>

typedef struct smp_cpu {
    uint8_t acpi_id;
    uint8_t lapic_id;
    bool is_bsp;
    bool init_failed;
    uint64_t *wake_on_write;
    struct smp_cpu *next;
} smp_cpu_t;

smp_cpu_t *smp_initialize_aps(acpi_sdt_header_t *madt_header, void *reserved_init_page, void *address_space);