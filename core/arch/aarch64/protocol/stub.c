#include "common/log.h"
#include "common/panic.h"

#include <stdint.h>

void linux_handoff() {
    panic("Tartarus does not support the Linux protocol on Aarch64");
}
