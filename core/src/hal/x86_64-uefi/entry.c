#include <stdint.h>
#include <cpuid.h>
#include <common/log.h>
#include <hal/uefi/efi.h>
#include <hal/x86_64/msr.h>

#define CPUID_NX (1 << 20)
#define MSR_EFER 0xC0000080
#define MSR_EFER_NX (1 << 11)

EFI_SYSTEM_TABLE *g_system_table;
EFI_HANDLE g_imagehandle;

bool g_cpu_nx_support = false;

static void log_sink(char c) {
    asm volatile("outb %0, %1" : : "a" (c), "Nd" (0x3F8));
}

[[noreturn]] EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    g_system_table = SystemTable;
    g_imagehandle = ImageHandle;

    log_sink('\n');
    log_init(log_sink);
    log("CORE", "Tartarus Bootloader (UEFI)");

    // Enable NX
    unsigned int edx = 0, unused;
    if(__get_cpuid(0x80000001, &unused, &unused, &unused, &edx) != 0) g_cpu_nx_support = (edx & CPUID_NX) != 0;
    if(g_cpu_nx_support) {
        msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_NX);
    } else {
        log_warning("CORE", "CPU does not support EFER.NXE");
    }

    for(;;);
}