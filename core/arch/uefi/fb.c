#include "arch/fb.h"

#include "common/panic.h"
#include "memory/heap.h"

#include "arch/uefi/uefi.h"

fb_t *arch_fb_acquire(uint32_t target_width, uint32_t target_height, bool strict_rgb) {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = g_uefi_system_table->BootServices->LocateProtocol(&gop_guid, NULL, (void **) &gop);
    if(EFI_ERROR(status)) return NULL;

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
    if(!closest_found) return NULL;
    status = gop->SetMode(gop, closest);
    if(EFI_ERROR(status)) return NULL;

    fb_t *fb = heap_alloc(sizeof(fb_t));
    fb->address = gop->Mode->FrameBufferBase;
    fb->size = gop->Mode->FrameBufferSize;
    fb->width = gop->Mode->Info->HorizontalResolution;
    fb->height = gop->Mode->Info->VerticalResolution;
    fb->pitch = gop->Mode->Info->PixelsPerScanLine * (32 / 8);

    fb->bpp = 32;
    fb->mask_red_size = 8;
    fb->mask_green_size = 8;
    fb->mask_blue_size = 8;
    switch(gop->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            fb->mask_red_position = 0;
            fb->mask_green_position = 8;
            fb->mask_blue_position = 16;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            fb->mask_red_position = 16;
            fb->mask_green_position = 8;
            fb->mask_blue_position = 0;
            break;
        case PixelBltOnly: panic("framebuffer only supports blit");
        case PixelBitMask:
        default:           panic("unsupported framebuffer pixel format");
    }
    return fb;
}
