#pragma once
#include <stdint.h>

typedef struct [[gnu::packed]] {
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem[6];
    uint8_t revision;
    uint32_t rsdt_address;
} acpi_rsdp_t;

typedef struct [[gnu::packed]] {
    acpi_rsdp_t rsdp;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} acpi_rsdp_ext_t;

typedef struct [[gnu::packed]] {
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} acpi_sdt_header_t;

acpi_sdt_header_t *acpi_find_table(acpi_rsdp_t *rsdp, const char *signature);