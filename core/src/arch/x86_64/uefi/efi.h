#pragma once

#include <efi.h>

extern EFI_SYSTEM_TABLE *g_x86_64_uefi_efi_system_table;
extern EFI_HANDLE g_x86_64_uefi_efi_image_handle;

void x86_64_uefi_efi_bootservices_exit();