#include "dev/dtb.h"

#include "arch/ptm.h"
#include "common/log.h"
#include "common/panic.h"
#include "dev/firmware.h"
#include "dev/virtio/virtio_mmio.h"
#include "memory/pmm.h"

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
    if(offset + length > DTB_ARENA_SIZE) return nullptr;
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

bool dtb_early_init(void *dtb_pointer) {
    dtb_ops ops = {
        .malloc = dtb_malloc,
        .free = dtb_free,
        .on_error = dtb_on_error,
    };

    return dtb_init((uintptr_t) dtb_pointer, ops);
}

static void get_cells(dtb_node *parent, smoldtb_value *addr_cells, smoldtb_value *size_cells) {
    *addr_cells = 2;
    *size_cells = 1;

    dtb_prop *p;
    if((p = dtb_find_prop(parent, "#address-cells"))) dtb_read_prop_1(p, 1, addr_cells);
    if((p = dtb_find_prop(parent, "#size-cells"))) dtb_read_prop_1(p, 1, size_cells);
}

static void for_each_reg(dtb_node *child, void (*fn)(uintptr_t base, size_t len)) {
    smoldtb_value addr_cells, size_cells;
    get_cells(dtb_get_parent(child), &addr_cells, &size_cells);

    dtb_prop *reg = dtb_find_prop(child, "reg");
    if(!reg) return;

    dtb_pair layout = {
        .a = addr_cells,
        .b = size_cells,
    };

    size_t pairs = dtb_read_prop_2(reg, layout, nullptr);
    dtb_pair *values = __builtin_alloca(pairs * sizeof(dtb_pair));

    dtb_read_prop_2(reg, layout, values);
    for(size_t i = 0; i < pairs; i++) fn((uintptr_t) values[i].a, (size_t) values[i].b);
}

typedef struct {
    const char *compatible;
    void (*register_device)(uintptr_t base, size_t len);
} dtb_driver_t;

// Matches "compatible" string in device tree
static const dtb_driver_t g_dtb_drivers[] = {
    {"virtio,mmio", virtio_mmio_register},
};

void dtb_init_devices() {
    for(size_t i = 0; i < sizeof(g_dtb_drivers) / sizeof(g_dtb_drivers[0]); i++) {
        const dtb_driver_t *driver = &g_dtb_drivers[i];

        for(dtb_node *node = dtb_find_compatible(nullptr, driver->compatible); node != nullptr; node = dtb_find_compatible(node, driver->compatible)) { for_each_reg(node, driver->register_device); }
    }
}

static bool check_padding(uint64_t base, uint64_t length) {
    uint64_t top = base + length;

    for(size_t i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *entry = &g_pmm_map[i];
        if(entry->base >= top || entry->base + entry->length <= base) { continue; }
        if(entry->type != PMM_MAP_TYPE_FREE && entry->type != PMM_MAP_TYPE_RESERVED) { return false; }
    }

    return true;
}

static void map_table(uintptr_t addr, size_t length) {
    uint64_t aligned_base = MATH_FLOOR(addr, PTM_PAGE_GRANULARITY);
    uint64_t aligned_top = MATH_CEIL(addr + length, PTM_PAGE_GRANULARITY);

    if(!check_padding(aligned_base, addr - aligned_base)) { aligned_base = addr; }
    if(!check_padding(addr + length, aligned_top - (addr + length))) { aligned_base = addr; }

    pmm_map_type_t map_type = -1;
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *entry = &g_pmm_map[i];
        if(addr >= entry->base && addr < entry->base + entry->length) {
            map_type = entry->type;
            break;
        }
    }

    if(map_type == PMM_MAP_TYPE_RESERVED) { pmm_map_set(aligned_base, aligned_top - aligned_base, PMM_MAP_TYPE_DEVICE_TREE, true); }
}

void dtb_map_device_tree() {
    uintptr_t address = (uintptr_t) firmware_get_dtb();
    size_t size = dtb_query_total_size(address);

    map_table(address, size);
}
