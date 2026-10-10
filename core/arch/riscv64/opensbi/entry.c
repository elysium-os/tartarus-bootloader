#include "arch/cpu.h"
#include "common/log.h"
#include "common/panic.h"
#include "core.h"
#include "dev/dtb.h"
#include "lib/string.h"
#include "memory/pmm.h"
#include "smoldtb.h"

#include "arch/riscv64/csr.h"
#include "arch/riscv64/sbi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern nullptr_t ld_tartarus_start[];
extern nullptr_t ld_tartarus_end[];

static log_sink_t g_sbi_putchar_sink = {.level = LOG_LEVEL_DEBUG, .char_out = sbi_legacy_putc};

static void get_cells(dtb_node *parent, size_t *addr_cells, size_t *size_cells) {
    *addr_cells = 2;
    *size_cells = 1;

    dtb_prop *p;
    if((p = dtb_find_prop(parent, "#address-cells"))) dtb_read_prop_1(p, 1, addr_cells);
    if((p = dtb_find_prop(parent, "#size-cells"))) dtb_read_prop_1(p, 1, size_cells);
}

static void for_each_reg(dtb_node *child, void (*fn)(uintptr_t base, size_t len)) {
    size_t addr_cells, size_cells;
    get_cells(dtb_get_parent(child), &addr_cells, &size_cells);

    dtb_prop *reg = dtb_find_prop(child, "reg");
    if(!reg) return;

    dtb_pair layout = {
        .a = addr_cells,
        .b = size_cells,
    };

    size_t pairs = dtb_read_prop_2(reg, layout, NULL);
    dtb_pair *values = __builtin_alloca(pairs * sizeof(dtb_pair));

    dtb_read_prop_2(reg, layout, values);
    for(size_t i = 0; i < pairs; i++) fn(values[i].a, values[i].b);
}

void tree_free_region(uintptr_t base, size_t len) {
    pmm_map_add(base, len, PMM_MAP_TYPE_FREE);
}

void tree_reserve_region(uintptr_t base, size_t len) {
    pmm_map_set(base, len, PMM_MAP_TYPE_RESERVED, true);
}

[[noreturn]] void riscv64_opensbi_entry(uint64_t hart_id, uint64_t dtb_pointer) {
    sbi_legacy_putc('\n');
    log_sink_add(&g_sbi_putchar_sink);

    log(LOG_LEVEL_DEBUG, "tartarus riscv <3");
    log(LOG_LEVEL_DEBUG, "hart = %lu, dtb = 0x%lx", hart_id, dtb_pointer);

    // @todo: work out a better way to store the hart_id, this is really lazy but it works
    ARCH_CSR_WRITE(sscratch, hart_id);

    if(!arch_dtb_early_init(dtb_pointer)) { panic("Failed to init device tree"); }

    dtb_node *root = dtb_find("/");

    for(dtb_node *node = dtb_get_child(root); node; node = dtb_get_sibling(node)) {
        dtb_prop *device_type = dtb_find_prop(node, "device_type");
        if(!device_type) continue;

        const char *type = dtb_read_prop_string(device_type, 0);
        if(!string_eq(type, "memory")) continue;

        for_each_reg(node, tree_free_region);
    }

    dtb_node *rsv = dtb_find("/reserved-memory");
    if(rsv) {
        for(dtb_node *c = dtb_get_child(rsv); c; c = dtb_get_sibling(c)) { for_each_reg(c, tree_reserve_region); }
    }

    // Claim tartarus and stack
    pmm_map_set((uintptr_t) ld_tartarus_start, (uintptr_t) ld_tartarus_end - (uintptr_t) ld_tartarus_start, PMM_MAP_TYPE_ALLOCATED, true);

    arch_dtb_init();

    core();
    arch_cpu_halt();
}
