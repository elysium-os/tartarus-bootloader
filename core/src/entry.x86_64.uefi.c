#include "common/log.h"

#include <efi.h>

static void qemu_debug_log(char c) {
    asm volatile("outb %0, %1" : : "a"(c), "Nd"(0x3F8));
}

[[noreturn]] EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    qemu_debug_log('\n');
    log_sink_set(qemu_debug_log);

    log(LOG_LEVEL_INFO, "Tartarus");

    for(;;);
}
