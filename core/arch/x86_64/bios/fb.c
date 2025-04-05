#include "arch/fb.h"

#include "lib/mem.h"
#include "memory/heap.h"

#include "arch/x86_64/bios/int.h"

#define MEMORY_MODEL_RGB 6
#define LINEAR_FRAMEBUFFER (1 << 7)
#define USE_LFB (1 << 14)

typedef struct [[gnu::packed]] {
    uint8_t signature[4];
    uint8_t version_minor;
    uint8_t version_major;
    uint16_t oem_offset;
    uint16_t oem_segment;
    uint32_t capabilities;
    uint16_t video_modes_offset;
    uint16_t video_modes_segment;
    uint16_t video_memory_blocks;
    uint16_t software_revision;
    uint16_t vendor_offset;
    uint16_t vendor_segment;
    uint16_t product_name_offset;
    uint16_t product_name_segment;
    uint16_t product_revision_offset;
    uint16_t product_revision_segment;
    uint8_t rsv0[222];
    uint8_t oem_data[256];
} vesa_vbe_info_t;

typedef struct [[gnu::packed]] {
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t rsv0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint16_t linear_pitch;
    uint8_t number_of_images_banked;
    uint8_t linear_number_of_images;
    uint8_t linear_red_mask_size;
    uint8_t linear_red_mask_position;
    uint8_t linear_green_mask_size;
    uint8_t linear_green_mask_position;
    uint8_t linear_blue_mask_size;
    uint8_t linear_blue_mask_position;
    uint8_t linear_reserved_mask_size;
    uint8_t linear_reserved_mask_position;
    uint32_t maximum_pixel_clock;
    uint8_t rsv1[190];
} vesa_vbe_mode_info_t;

static inline bool get_mode_info(uint16_t mode, vesa_vbe_mode_info_t *dest) {
    int_regs_t regs = {.eax = 0x4F01, .ecx = mode, .es = INT_16BIT_SEGMENT(dest), .edi = INT_16BIT_OFFSET(dest)};
    int_exec(0x10, &regs);
    return (regs.eax & 0xFFFF) != 0x4F;
}

fb_t *arch_fb_acquire(uint32_t target_width, uint32_t target_height, bool strict_rgb) {
    vesa_vbe_info_t vbe_info = {};
    ((uint8_t *) &vbe_info)[0] = (uint8_t) 'V';
    ((uint8_t *) &vbe_info)[1] = (uint8_t) 'B';
    ((uint8_t *) &vbe_info)[2] = (uint8_t) 'E';
    ((uint8_t *) &vbe_info)[3] = (uint8_t) '2';

    int_regs_t regs = {.eax = 0x4F00, .es = INT_16BIT_SEGMENT(&vbe_info), .edi = INT_16BIT_OFFSET(&vbe_info)};
    int_exec(0x10, &regs);
    if((regs.eax & 0xFFFF) != 0x4F) return NULL;
    bool closest_found = false;
    uint16_t closest = 0;
    uint64_t closest_diff = UINT64_MAX;

    uint16_t *modes = (uint16_t *) INT_16BIT_DESEGMENT(vbe_info.video_modes_segment, vbe_info.video_modes_offset);
    vesa_vbe_mode_info_t mode_info;
    while(*modes != 0xFFFF) {
        uint16_t mode = *modes++;
        if(get_mode_info(mode, &mode_info)) continue;
        if(mode_info.memory_model != MEMORY_MODEL_RGB) continue;
        if(!(mode_info.attributes & LINEAR_FRAMEBUFFER)) continue;

        bool is_strict_rgb = false;
        if(mode_info.red_mask == 8 && mode_info.blue_mask == 8 && mode_info.green_mask == 8 && mode_info.reserved_mask == 8 && mode_info.red_position == 16 && mode_info.green_position == 8 &&
           mode_info.blue_position == 0 && mode_info.reserved_position == 24 && mode_info.bpp == 32)
            is_strict_rgb = true;
        if(strict_rgb && !is_strict_rgb) continue;

        int64_t signed_diff = ((int64_t) target_width - (int64_t) mode_info.width) + ((int64_t) target_height - (int64_t) mode_info.height);
        uint64_t diff = signed_diff < 0 ? -signed_diff : signed_diff;
        if(diff > closest_diff || (diff == closest_diff && !is_strict_rgb)) continue;
        closest_found = true;
        closest_diff = diff;
        closest = mode;
    }
    if(!closest_found) return NULL;

    memset(&regs, 0, sizeof(int_regs_t));
    regs.eax = 0x4F02;
    regs.ebx = closest | USE_LFB;
    int_exec(0x10, &regs);
    if((regs.eax & 0xFFFF) != 0x4F) return NULL;

    get_mode_info(closest, &mode_info);

    fb_t *fb = heap_alloc(sizeof(fb_t));
    fb->address = mode_info.framebuffer;
    if(vbe_info.version_major < 3) {
        fb->pitch = mode_info.pitch;
        fb->mask_red_size = mode_info.red_mask;
        fb->mask_red_position = mode_info.red_position;
        fb->mask_green_size = mode_info.green_mask;
        fb->mask_green_position = mode_info.green_position;
        fb->mask_blue_size = mode_info.blue_mask;
        fb->mask_blue_position = mode_info.blue_position;
    } else {
        fb->pitch = mode_info.linear_pitch;
        fb->mask_red_size = mode_info.linear_red_mask_size;
        fb->mask_red_position = mode_info.linear_red_mask_position;
        fb->mask_green_size = mode_info.linear_green_mask_size;
        fb->mask_green_position = mode_info.linear_green_mask_position;
        fb->mask_blue_size = mode_info.linear_blue_mask_size;
        fb->mask_blue_position = mode_info.linear_blue_mask_position;
    }
    fb->bpp = mode_info.bpp;
    fb->width = mode_info.width;
    fb->height = mode_info.height;
    fb->size = fb->height * fb->pitch;
    return fb;
}
