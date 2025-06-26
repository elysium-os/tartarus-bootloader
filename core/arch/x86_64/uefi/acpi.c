#include "arch/acpi.h"

#include "lib/mem.h"

#include "arch/x86_64/uefi/uefi.h"

static bool compare_guid(EFI_GUID *a, EFI_GUID *b) {
    return memcmp(a, b, sizeof(EFI_GUID)) == 0;
}

static bool checksum(void *src, size_t count) {
    uint8_t sum = 0;
    for(size_t i = 0; i < count; i++) sum += ((uint8_t *) src)[i];
    return sum == 0;
}

void *arch_acpi_find_rsdp() {
    EFI_GUID v1 = ACPI_TABLE_GUID;
    EFI_GUID v2 = ACPI_20_TABLE_GUID;

    void *rsdp = NULL;
    for(UINTN i = 0; i < g_x86_64_uefi_efi_system_table->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &g_x86_64_uefi_efi_system_table->ConfigurationTable[i];

        if(compare_guid(&table->VendorGuid, &v1) && checksum(table->VendorTable, sizeof(acpi_rsdp_t))) rsdp = table->VendorTable;
        if(compare_guid(&table->VendorGuid, &v2) && checksum(table->VendorTable, sizeof(acpi_rsdp_ext_t))) return table->VendorTable;
    }

    return rsdp;
}
