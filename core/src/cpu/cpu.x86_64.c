#include "cpu.h"

void cpu_halt() {
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}
