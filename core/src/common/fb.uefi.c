#include "fb.h"
#include <sys/efi.uefi.h>

bool fb_acquire(uint32_t target_width, uint32_t target_height, bool strict_rgb, fb_t *out) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = g_system_table->BootServices->LocateProtocol(&gop_guid, NULL, (void **) &gop);
    if(EFI_ERROR(status)) return true;

    bool closest_found = false;
    UINTN closest = 0;
    UINT32 closest_difference = UINT32_MAX;
    for(UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
        UINTN info_size;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
        status = gop->QueryMode(gop, i, &info_size, &info);
        if(EFI_ERROR(status)) continue;
        if(strict_rgb && info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor) continue;
        UINT32 difference = info->HorizontalResolution > target_width ? info->HorizontalResolution - target_width : target_width - info->HorizontalResolution;
        difference += info->VerticalResolution > target_height ? info->VerticalResolution - target_height : target_height - info->VerticalResolution;
        if(difference > closest_difference || (difference == closest_difference && info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)) continue;
        closest_found = true;
        closest_difference = difference;
        closest = i;
    }
    if(!closest_found) return true;
    status = gop->SetMode(gop, closest);
    if(EFI_ERROR(status)) return true;
    out->address = gop->Mode->FrameBufferBase;
    out->size = gop->Mode->FrameBufferSize;
    out->width = gop->Mode->Info->HorizontalResolution;
    out->height = gop->Mode->Info->VerticalResolution;
    out->pitch = gop->Mode->Info->PixelsPerScanLine;
    return false;
}