#pragma once

#include <stdint.h>

typedef struct [[gnu::packed]] {
    uint16_t low_limit;
    uint16_t low_base;
    uint8_t mid_base;
    uint8_t access_byte;
    uint8_t flags_upper_limit;
    uint8_t upper_base;
} gdt_entry_t;

extern gdt_entry_t g_gdt[];
extern uint16_t g_gdt_limit;