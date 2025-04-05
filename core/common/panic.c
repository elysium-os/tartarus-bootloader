#include "panic.h"

#include "arch/cpu.h"
#include "common/log.h"

#include <stdarg.h>

[[noreturn]] void panic(const char *fmt, ...) {
    va_list list;
    va_start(list, str);
    log_list(LOG_LEVEL_ERROR, fmt, list);
    va_end(list);
    for(;;) arch_cpu_halt();
    __builtin_unreachable();
}
