#include "lib/mem.h"

void *arch_dtb_find_dtb() {
    // we set the device tree in riscv64/opensbi/entry.c
    return nullptr;
}
