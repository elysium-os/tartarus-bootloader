#include "arch/fb.h"

fb_t *arch_fb_acquire(uint32_t target_width, uint32_t target_height, bool strict_rgb) {
    (void) target_width;
    (void) target_height;
    (void) strict_rgb;
    return nullptr;
}
