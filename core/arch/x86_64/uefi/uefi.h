#pragma once

#include "common/log.h"

#include <efi.h>

extern EFI_SYSTEM_TABLE *g_x86_64_uefi_efi_system_table;
extern EFI_HANDLE g_x86_64_uefi_efi_image_handle;

extern log_sink_t g_uefi_sink;

void x86_64_uefi_efi_bootservices_exit();
