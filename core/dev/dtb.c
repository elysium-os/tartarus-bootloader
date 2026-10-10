#include "dev/dtb.h"

#include "common/log.h"
#include "common/panic.h"
#include "dev/virtio/virtio_mmio.h"
#include "lib/string.h"

#include <smoldtb.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DTB_ARENA_SIZE (256 * 1024)

// device trees need to be usable *before* memory map init
static uint8_t g_dtb_arena[DTB_ARENA_SIZE];
static size_t g_dtb_arena_used;

static void *dtb_malloc(size_t length) {
    size_t offset = (g_dtb_arena_used + 15) & ~(size_t) 15;
    if(offset + length > DTB_ARENA_SIZE) return NULL;
    g_dtb_arena_used = offset + length;
    return &g_dtb_arena[offset];
}

static void dtb_free(void *ptr, size_t length) {
    (void) ptr;
    (void) length;
}

static void dtb_on_error(const char *why) {
    panic("dtb: %s", why);
}

uintptr_t g_dtb_pointer;

bool arch_dtb_early_init(uintptr_t dtb_pointer) {
    dtb_ops ops = {
        .malloc = dtb_malloc,
        .free = dtb_free,
        .on_error = dtb_on_error,
    };

    g_dtb_pointer = dtb_pointer;
    return dtb_init(dtb_pointer, ops);
}

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


void arch_dtb_init() {
    dtb_node *soc = dtb_find("/soc");
    for(dtb_node *node = dtb_get_child(soc); node != NULL; node = dtb_get_sibling(node)) {
        dtb_node_stat stat;

        if(!dtb_stat_node(node, &stat)) continue;

        if(string_ncmp(stat.name, "virtio_mmio@", 11) == 0) { for_each_reg(node, virtio_mmio_register); }
    }
}

uintptr_t arch_dtb_get() {
    return g_dtb_pointer;
}
