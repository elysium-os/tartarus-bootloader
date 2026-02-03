#pragma once

#include "dev/acpi.h"

#include <stdint.h>

typedef struct [[gnu::packed]] {
    acpi_sdt_header_t sdt_header;
    uint32_t local_apic_address;
    uint32_t flags;
} madt_t;

typedef enum {
    MADT_LAPIC = 0,
    MADT_IOAPIC,
    MADT_SOURCE_OVERRIDE,
    MADT_NMI_SOURCE,
    MADT_NMI,
    MADT_LAPIC_ADDRESS,
    MADT_LX2APIC = 9,
    MADT_GICC = 11
} madt_record_types_t;

typedef struct [[gnu::packed]] {
    uint8_t type;
    uint8_t length;
} madt_record_t;

typedef struct [[gnu::packed]] {
    madt_record_t base;
    uint8_t acpi_processor_id;
    uint8_t lapic_id;
    uint32_t flags;
} madt_record_lapic_t;

typedef struct [[gnu::packed]] {
    madt_record_t base;
    uint16_t rsv0;
    uint32_t cpu_interface_number;
    uint32_t acpi_processor_id;
    uint32_t flags;
    uint32_t parking_protocol_version;
    uint32_t performance_interrupt_gsi;
    uint64_t parked_address;
    uint64_t physical_base_address;
    uint64_t gicv;
    uint64_t gich;
    uint32_t vgic_maintenance_interrupt;
    uint64_t gicr_base_address;
    uint64_t mpidr;
    uint8_t processor_power_efficiency_class;
} madt_record_gicc_t;
