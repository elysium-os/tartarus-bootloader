[[noreturn]] void arch_cpu_halt() {
    for(;;) asm volatile("wfi");
    __builtin_unreachable();
}

void arch_cpu_init() {}
