#include "gdt.h"

// clang-format off
x86_64_gdt_entry_t g_x86_64_gdt[] = {
    {},
    { // code 16
        .low_limit = 0xFFFF,
        .access_byte = 0b10011011,
        .flags_upper_limit = 0b00001111
    },
    { // data 16
        .low_limit = 0xFFFF,
        .access_byte = 0b10010011,
        .flags_upper_limit = 0b00001111
    },
    { // code 32
        .low_limit = 0xFFFF,
        .access_byte = 0b10011011,
        .flags_upper_limit = 0b11001111
    },
    { // data 32
        .low_limit = 0xFFFF,
        .access_byte = 0b10010011,
        .flags_upper_limit = 0b11001111
    },
    { // code 64
        .access_byte = 0b10011011,
        .flags_upper_limit = 0b00100000
    },
    { // data 64
        .access_byte = 0b10010011
    }
};
// clang-format on

#ifdef __PLATFORM_X86_64_BIOS
__attribute__((section(".early")))
#endif
uint16_t g_x86_64_gdt_limit = sizeof(g_x86_64_gdt) - 1;
