#include "common/log.h"
#include "cpu/cpu.x86_64.h"
#ifdef __ENV_DEVELOPMENT
static void qemu_debug_log(char c) {
    asm volatile("outb %0, %1" : : "a"(c), "Nd"(0x3F8));
}
#endif

[[noreturn]] void entry() {
#ifdef __ENV_DEVELOPMENT
    qemu_debug_log('\n');
    log_sink_set(qemu_debug_log);
#endif

    cpu_enable_nx();
    for(;;);
}
