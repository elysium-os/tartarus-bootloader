#include <hal/cpu.h>

void hal_cpu_halt() {
    asm volatile("hlt");
}