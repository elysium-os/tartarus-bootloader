#include "arch/time.h"

#include "common/log.h"

time_t arch_time() {
    log(LOG_LEVEL_WARN, "riscv64-opensbi, time is unimplemented...");
    return 0;
}
