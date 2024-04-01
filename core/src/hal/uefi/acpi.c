#include <hal/acpi.h>
#include <hal/uefi/efi.h>

static bool compare_guid(EFI_GUID a, EFI_GUID b) {
    bool data4_match = true;
    for(int i = 0; i < 8; i++) if(a.Data4[i] != b.Data4[i]) data4_match = false;
    return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3 && data4_match;
}

static uintptr_t find_table(EFI_GUID guid) {
    EFI_CONFIGURATION_TABLE *table = g_system_table->ConfigurationTable;
    for(UINTN i = 0; i < g_system_table->NumberOfTableEntries; i++) {
        if(compare_guid(table->VendorGuid, guid)) return (uintptr_t) table->VendorTable;
        table++;
    }
    return 0;
}

acpi_rsdp_t *hal_acpi_find_rsdp() {
    EFI_GUID v1 = ACPI_TABLE_GUID;
    EFI_GUID v2 = ACPI_20_TABLE_GUID;
    uintptr_t address = find_table(v2);
    if(!address) address = find_table(v1);
    return (acpi_rsdp_t *) address;
}