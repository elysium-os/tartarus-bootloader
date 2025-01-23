#include "common/log.h"
#include "core.h"
#include "cpu/cpu.x86_64.h"
#include "cpu/int.x86_64.bios.h"
#include "memory/pmm.h"

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

#ifdef __ENV_DEVELOPMENT
static void qemu_debug_log(char c) {
    asm volatile("outb %0, %1" : : "a"(c), "Nd"(0x3F8));
}
#endif

[[noreturn]] void entry() {
#ifdef __ENV_DEVELOPMENT
    qemu_debug_log('\n');
    log_sink_set(qemu_debug_log);
#endif

    cpu_enable_nx();
    for(;;);

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

    core();
}
