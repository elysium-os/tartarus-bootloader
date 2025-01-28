#include "efi.h"

#include "common/panic.h"
#include "memory/heap.h"

void x86_64_uefi_efi_bootservices_exit() {
    UINTN umap_size = 0;
    EFI_MEMORY_DESCRIPTOR *umap = NULL;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
    EFI_STATUS status = g_x86_64_uefi_efi_system_table->BootServices->GetMemoryMap(&umap_size, umap, &map_key, &descriptor_size, &descriptor_version);
    if(status == EFI_BUFFER_TOO_SMALL) {
        umap = heap_alloc(umap_size);
        status = g_x86_64_uefi_efi_system_table->BootServices->GetMemoryMap(&umap_size, umap, &map_key, &descriptor_size, &descriptor_version);
        if(EFI_ERROR(status)) panic("unable retrieve the UEFI memory map for bootservices exit");
    }
    g_x86_64_uefi_efi_system_table->BootServices->ExitBootServices(g_x86_64_uefi_efi_image_handle, map_key);
}
