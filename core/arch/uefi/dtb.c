#include "lib/mem.h"

#include "arch/uefi/uefi.h"

static bool compare_guid(EFI_GUID *a, EFI_GUID *b) {
    return memcmp(a, b, sizeof(EFI_GUID)) == 0;
}

void *arch_dtb_find_dtb() {
    EFI_GUID dtb = EFI_DTB_TABLE_GUID;

    void *rsdp = NULL;
    for(UINTN i = 0; i < g_uefi_system_table->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE *table = &g_uefi_system_table->ConfigurationTable[i];

        if(compare_guid(&table->VendorGuid, &dtb)) return table->VendorTable;
    }

    return rsdp;
}
