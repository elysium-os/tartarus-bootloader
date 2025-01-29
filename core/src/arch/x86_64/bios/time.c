#include "arch/time.h"

#include "arch/x86_64/bios/int.h"

#define EFLAGS_CF (1 << 0)

static uint8_t convert_bcd(uint8_t bcd) {
    return (bcd & 0x0F) + ((bcd & 0xF0) >> 4) * 10;
}

time_t arch_time() {
    int_regs_t regs;

restart:
    regs = (int_regs_t) {.eax = (0x4 << 8)};
    int_exec(0x1A, &regs);
    if(regs.eflags & EFLAGS_CF) return 0;
    uint16_t year = convert_bcd((uint8_t) regs.ecx) + convert_bcd((uint8_t) (regs.ecx >> 8)) * 100;
    uint8_t month = convert_bcd((uint8_t) (regs.edx >> 8));
    uint8_t day = convert_bcd((uint8_t) regs.edx);

    regs = (int_regs_t) {.eax = (0x2 << 8)};
    int_exec(0x1A, &regs);
    if(regs.eflags & EFLAGS_CF) return 0;
    uint8_t hour = convert_bcd((uint8_t) (regs.ecx >> 8));
    uint8_t minute = convert_bcd((uint8_t) regs.ecx);
    uint8_t second = convert_bcd((uint8_t) (regs.edx >> 8));

    regs = (int_regs_t) {.eax = (0x4 << 8)};
    int_exec(0x1A, &regs);
    if(regs.eflags & EFLAGS_CF) return 0;
    if(convert_bcd((uint8_t) regs.edx) != day) goto restart;

    return time_datetime_to_unix_time(year, month, day, hour, minute, second);
}
