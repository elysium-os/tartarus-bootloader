#include "common/log.h"
#include "core.h"
#include "memory/pmm.h"

#include "arch/x86_64/bios/int.h"
#include "arch/x86_64/cpu.h"

#define E820_MAX 512
#define E820_MAGIC_NUMBER 0x534D4150

typedef struct [[gnu::packed]] {
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi3attr;
} e820_entry_t;

typedef enum {
    E820_TYPE_USABLE = 1,
    E820_TYPE_RESERVED,
    E820_TYPE_ACPI_RECLAIMABLE,
    E820_TYPE_ACPI_NVS,
    E820_TYPE_BAD,
} e820_type_t;

extern nullptr_t ld_tartarus_start[];
extern nullptr_t ld_tartarus_end[];

#ifdef __BUILD_DEBUG
static void qemu_debug_log(char ch) {
    asm volatile("outb %0, %1" : : "a"(ch), "Nd"(0xE9));
}

static log_sink_t g_qemu_debug_sink = {.level = LOG_LEVEL_DEBUG, .char_out = qemu_debug_log};
#endif

[[noreturn]] void x86_64_bios_entry() {
#ifdef __BUILD_DEBUG
    qemu_debug_log('\n');
    log_sink_add(&g_qemu_debug_sink);
#endif

    x86_64_cpu_init();

    // Load E820
    e820_entry_t e820[E820_MAX];
    int e820_size = 0;
    int_regs_t regs = {.edi = (uint32_t) &e820};
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
        log(LOG_LEVEL_DEBUG, "e820[%i] = { base: %#llx, length: %#llx, type: %u }", i, e820[i].address, e820[i].length, e820[i].type);
        pmm_map_type_t type;
        switch(e820[i].type) {
            case E820_TYPE_USABLE:           type = PMM_MAP_TYPE_FREE; break;
            case E820_TYPE_ACPI_RECLAIMABLE: type = PMM_MAP_TYPE_ACPI_RECLAIMABLE; break;
            case E820_TYPE_ACPI_NVS:         type = PMM_MAP_TYPE_ACPI_NVS; break;
            case E820_TYPE_BAD:              type = PMM_MAP_TYPE_BAD; break;
            case E820_TYPE_RESERVED:
            default:                         type = PMM_MAP_TYPE_RESERVED; break;
        }
        pmm_map_add(e820[i].address, e820[i].length, type);
    }

    // Claim tartarus and stack
    pmm_map_set(0x1000, 0x7000, PMM_MAP_TYPE_ALLOCATED, true);
    pmm_map_set((uintptr_t) ld_tartarus_start, (uintptr_t) ld_tartarus_end - (uintptr_t) ld_tartarus_start, PMM_MAP_TYPE_ALLOCATED, true);

    core();
}
