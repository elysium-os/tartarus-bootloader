#pragma once

#include "dev/acpi.h"

typedef struct [[gnu::packed]] {
    acpi_sdt_header_t sdt_header;
    uint32_t flags;
    uint64_t time_base_frequency;
    uint32_t node_count;
    uint32_t nodes_offset;
} rhct_t;

typedef enum {
    ACPI_RHCT_ENTRY_TYPE_MMU = 2
} rhct_node_type_t;

typedef struct [[gnu::packed]] {
    uint16_t type;
    uint16_t length;
} rhct_node_t;

typedef enum : uint8_t {
    ACPI_RHCT_MMU_TYPE_SV39 = 0,
    ACPI_RHCT_MMU_TYPE_SV48 = 1,
    ACPI_RHCT_MMU_TYPE_SV57 = 2
} rhct_node_mmu_type_t;

typedef struct [[gnu::packed]] {
    rhct_node_t node;
    uint16_t revision;
    uint8_t rsv0;
    rhct_node_mmu_type_t mmu_type;
} rhct_node_mmu_t;
