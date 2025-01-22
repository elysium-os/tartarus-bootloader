#include "panic.h"

#include "common/log.h"
#include "cpu/cpu.h"

#include <stdarg.h>

[[noreturn]] void panic(const char *fmt, ...) {
    va_list list;
    va_start(list, str);
    log_list(LOG_LEVEL_ERROR, fmt, list);
    va_end(list);
    for(;;) cpu_halt();
    __builtin_unreachable();
}
