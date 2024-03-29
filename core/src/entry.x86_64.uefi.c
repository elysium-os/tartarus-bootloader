#include <stdint.h>
#include <common/log.h>

static void log_sink(char c) {
    asm volatile("outb %0, %1" : : "a" (c), "Nd" (0x3F8));
}

[[noreturn]] void entry() {
    log_sink('\n');
    log_init(log_sink);
    log("CORE", "Tartarus Bootloader (UEFI)");

    for(;;);
}