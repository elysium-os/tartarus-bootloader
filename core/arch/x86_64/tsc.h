#pragma once

#include <stdint.h>

uint64_t x86_64_tsc_read();
void x86_64_tsc_block(uint64_t cycles);