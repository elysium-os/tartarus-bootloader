#include "tsc.h"

static inline uint64_t read() {
    uint32_t high, low;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t) high << 32) | low;
}

uint64_t x86_64_tsc_read() {
    return read();
}

void x86_64_tsc_block(uint64_t cycles) {
    uint64_t target = read() + cycles;
    while(read() < target);
}
