#include <stdint.h>
#include <cpuid.h>
#include <lib/symbol.h>
#include <common/log.h>
#include <core.h>
#include <memory/pmm.h>
#include <hal/x86_64/msr.h>
#include <hal/x86_64-bios/int.h>

#define E820_MAX 512
#define E820_MAGIC_NUMBER 0x534D4150
#define CPUID_NX (1 << 20)
#define MSR_EFER 0xC0000080
#define MSR_EFER_NX (1 << 11)

typedef struct {
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi3attr;
} __attribute__((packed)) e820_entry_t;

typedef enum {
    E820_TYPE_USABLE = 1,
    E820_TYPE_RESERVED,
    E820_TYPE_ACPI_RECLAIMABLE,
    E820_TYPE_ACPI_NVS,
    E820_TYPE_BAD,
} e820_type_t;

extern symbol_t ld_tartarus_start;
extern symbol_t ld_tartarus_end;

bool g_cpu_nx_support = false;

static void log_sink(char c) {
    asm volatile("outb %0, %1" : : "a" (c), "Nd" (0x3F8));
}

[[noreturn]] void entry() {
    log_sink('\n');
    log_init(log_sink);
    log("CORE", "Tartarus Bootloader (BIOS)");

    // Enable NX
    unsigned int edx = 0, unused;
    if(__get_cpuid(0x80000001, &unused, &unused, &unused, &edx) != 0) g_cpu_nx_support = (edx & CPUID_NX) != 0;
    if(g_cpu_nx_support) {
        msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_NX);
    } else {
        log_warning("CORE", "CPU does not support EFER.NXE");
    }

    // Load E820
    e820_entry_t e820[E820_MAX];
    int e820_size = 0;
    int_regs_t regs = { .edi = (uint32_t) &e820 };
    while(e820_size < E820_MAX) {
        *((uint32_t *) (uintptr_t) (regs.edi + 20)) = 1;
        regs.eax = 0xE820;
        regs.ecx = 24;
        regs.edx = E820_MAGIC_NUMBER;
        int_exec(0x15, &regs);

        e820_size++;
        if(regs.eax != E820_MAGIC_NUMBER || (regs.eflags & INT_REGS_EFLAGS_CF) || regs.ebx == 0) break;
        regs.edi += 24;
    }

    // Setup physical memory
    for(int i = 0; i < e820_size; i++) {
        pmm_map_type_t type;
        switch(e820[i].type) {
            case E820_TYPE_USABLE: type = PMM_MAP_TYPE_FREE; break;
            case E820_TYPE_ACPI_RECLAIMABLE: type = PMM_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case E820_TYPE_ACPI_NVS: type = PMM_MAP_TYPE_ACPI_NVS; break;
            case E820_TYPE_BAD: type = PMM_MAP_TYPE_BAD; break;
            case E820_TYPE_RESERVED:
            default: type = PMM_MAP_TYPE_RESERVED; break;
        }
        pmm_init_add(e820[i].address, e820[i].length, type);
    }
    log("CORE", "Initialized physical memory (%i memory map entries)", e820_size);

    // Protect initial stack & tartarus
    // TODO: the stack claim is flimsy
    if(pmm_convert(PMM_MAP_TYPE_FREE, PMM_MAP_TYPE_ALLOCATED, 0x1000, 0x7000)) log_panic("CORE", "Failed to claim stack memory");
    if(pmm_convert(PMM_MAP_TYPE_FREE, PMM_MAP_TYPE_ALLOCATED, (uintptr_t) ld_tartarus_start, (uintptr_t) ld_tartarus_end - (uintptr_t) ld_tartarus_start)) log_panic("CORE", "Failed to claim kernel memory");

    core();
    __builtin_unreachable();
}