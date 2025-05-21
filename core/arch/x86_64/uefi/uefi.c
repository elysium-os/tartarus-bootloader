#include "uefi.h"

#include "common/panic.h"
#include "memory/heap.h"

void uefi_char_out(char ch) {
    if(ch == '\n') uefi_char_out('\r');

    CHAR16 str[2] = {ch, 0};
    g_x86_64_uefi_efi_system_table->ConOut->OutputString(g_x86_64_uefi_efi_system_table->ConOut, str);
}

log_sink_t g_uefi_sink = {.level = LOG_LEVEL_DEBUG, .char_out = uefi_char_out};

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

    log_sink_remove(&g_uefi_sink);

    if(EFI_ERROR(g_x86_64_uefi_efi_system_table->BootServices->ExitBootServices(g_x86_64_uefi_efi_image_handle, map_key)))
        panic("failed to exit bootservices"); // TODO: to do this properly we should retry a couple times
}
