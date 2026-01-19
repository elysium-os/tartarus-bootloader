#pragma once

#include "common/log.h"

#include <efi.h>

extern EFI_SYSTEM_TABLE *g_uefi_system_table;
extern EFI_HANDLE g_uefi_image_handle;

extern log_sink_t g_uefi_log_sink;

void uefi_bootservices_exit();
