#pragma once
#include <stdint.h>

bool arch_dtb_early_init(uintptr_t dtb_pointer);
void arch_dtb_init();
uintptr_t arch_dtb_get();
