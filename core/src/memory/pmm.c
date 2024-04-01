#include "pmm.h"
#include <lib/math.h>
#include <common/log.h>

#define TYPE_IS_STRICT(TYPE) ((TYPE) == PMM_MAP_TYPE_FREE || (TYPE) == PMM_MAP_TYPE_ALLOCATED)
#define ENTRY_END(ENTRY) (entry->base + entry->length)

uint16_t g_pmm_map_size = 0;
pmm_map_entry_t g_pmm_map[PMM_MAX_MAP_ENTRIES];

static void map_insert(int index, pmm_map_entry_t entry) {
    if(g_pmm_map_size == PMM_MAX_MAP_ENTRIES) log_panic("PMM", "Memory map overflow");
    for(int i = g_pmm_map_size; i > index; i--) g_pmm_map[i] = g_pmm_map[i - 1];
    g_pmm_map[index] = entry;
    g_pmm_map_size++;
}

static void map_delete(int index) {
    for(int i = index; i < g_pmm_map_size - 1; i++) {
        g_pmm_map[i] = g_pmm_map[i + 1];
    }
    g_pmm_map_size--;
}

static int map_add_sorted(pmm_map_entry_t entry) {
    int i = 0;
    for(; i < g_pmm_map_size && g_pmm_map[i].base <= entry.base; i++);
    map_insert(i, entry);
    return i;
}

void pmm_init_add(uint64_t base, uint64_t length, pmm_map_type_t type) {
    if(TYPE_IS_STRICT(type)) {
        uint64_t aligned_base = MATH_CEIL(base, PMM_PAGE_SIZE);
        uint64_t base_diff = aligned_base - base;
        if(base_diff >= length) return;
        base = aligned_base;
        length = MATH_FLOOR(length - base_diff, PMM_PAGE_SIZE);

        if(base == 0) {
            if(length < PMM_PAGE_SIZE) return;
            base += PMM_PAGE_SIZE;
            length -= PMM_PAGE_SIZE;
        }
    }
    if(length == 0) return;

    restart:
    for(int i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *entry = &g_pmm_map[i];

        if(entry->base <= base && ENTRY_END(entry) > base) {
            if(type > entry->type) {
                if(ENTRY_END(entry) > base + length) {
                    uint64_t new_entry_base = base + length;
                    uint64_t new_entry_length = ENTRY_END(entry) - (base + length);
                    entry->length -= new_entry_length;
                    pmm_init_add(new_entry_base, new_entry_length, entry->type);
                }
                uint64_t new_length = base - entry->base;
                if(new_length == 0) {
                    map_delete(i);
                    goto restart;
                }
                entry->length = new_length;
            } else {
                uint64_t diff = ENTRY_END(entry) - base;
                if(diff >= length) return;
                base = ENTRY_END(entry);
                length -= diff;
            }
        }

        if(base < entry->base && entry->base < base + length) {
            if(type > entry->type) {
                uint64_t diff = (base + length) - entry->base;
                if(diff >= entry->length) {
                    map_delete(i);
                    goto restart;
                }
                entry->base = base + length;
                entry->length -= diff;
            } else {
                if(base + length > ENTRY_END(entry)) {
                    uint64_t new_entry_base = ENTRY_END(entry);
                    uint64_t new_entry_length = (base + length) - ENTRY_END(entry);
                    length -= new_entry_length;
                    pmm_init_add(new_entry_base, new_entry_length, type);
                }
                uint64_t new_length = entry->base - base;
                if(new_length == 0) {
                    map_delete(i);
                    goto restart;
                }
                length = new_length;
            }
        }
    }

    for(int i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *entry = &g_pmm_map[i];
        if(entry->type != type) continue;
        if(base == ENTRY_END(entry)) {
            entry->length += length;
            return;
        }
        if(base + length == entry->base) {
            entry->base = base;
            entry->length += length;
            return;
        }
    }

    map_add_sorted((pmm_map_entry_t) { .base = base, .length = length, .type = type });
}

bool pmm_convert(pmm_map_type_t src_type, pmm_map_type_t dest_type, uint64_t base, uint64_t length) {
    if(length == 0) log_panic("PMM", "Tried to create a zero-length entry");
    if(TYPE_IS_STRICT(dest_type) && base % PMM_PAGE_SIZE + length % PMM_PAGE_SIZE != 0) log_panic("PMM", "Tried to convert a non-aligned region to a strict type");
    for(int i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].type != src_type) continue;
        if(g_pmm_map[i].base > base || g_pmm_map[i].base + g_pmm_map[i].length < base + length) continue;

        uint64_t end_diff = (g_pmm_map[i].base + g_pmm_map[i].length) - (base + length);
        if(end_diff > 0) {
            map_add_sorted((pmm_map_entry_t) {
                .base = base + length,
                .length = end_diff,
                .type = g_pmm_map[i].type
            });
            g_pmm_map[i].length -= end_diff;
        }

        uint64_t start_diff = base - g_pmm_map[i].base;
        if(start_diff > 0) {
            g_pmm_map[i].length = start_diff;
            i = map_add_sorted((pmm_map_entry_t) {
                .base = base,
                .length = length,
                .type = dest_type
            });
        } else {
            g_pmm_map[i].type = dest_type;
        }

        for(int j = 0; j < g_pmm_map_size; j++) {
            if(j == i) continue;
            if(g_pmm_map[j].type != dest_type) continue;
            if(g_pmm_map[i].base + g_pmm_map[i].length == g_pmm_map[j].base) {
                g_pmm_map[i].length += g_pmm_map[j].length;
                map_delete(j);
            }
            if(g_pmm_map[j].base + g_pmm_map[j].length == g_pmm_map[i].base) {
                g_pmm_map[j].length += g_pmm_map[i].length;
                map_delete(i);
                i = j;
            }
        }

        return false;
    }
    return true;
}

void *pmm_alloc_ext(pmm_map_area_t area, pmm_map_type_t src_type, size_t page_count) {
    size_t length = page_count * PMM_PAGE_SIZE;
    for(int i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].type != src_type) continue;
        if(g_pmm_map[i].base + g_pmm_map[i].length <= area.start) continue; // entry ends before area start
        if(g_pmm_map[i].base >= area.end) continue; // entry starts after area end

        uintptr_t ue_base = g_pmm_map[i].base;
        if(g_pmm_map[i].base < area.start) ue_base = area.start;
        uintptr_t ue_length = g_pmm_map[i].length - (ue_base - g_pmm_map[i].base);
        if(ue_base + ue_length > area.end) ue_length -= (ue_base + ue_length) - area.end;
        if(ue_length < length) continue; // claim does not fit inside entry

        if(pmm_convert(src_type, PMM_MAP_TYPE_ALLOCATED, ue_base, length)) log_panic("PMM", "Claim failed because area was not valid (?)");
        return (void *) ue_base;
    }
    log_panic("PMM", "Out of memory");
    __builtin_unreachable();
}

void *pmm_alloc(pmm_map_area_t area, size_t page_count) {
    return pmm_alloc_ext(area, PMM_MAP_TYPE_FREE, page_count);
}

void pmm_free(void *address, size_t page_count) {
    if(pmm_convert(PMM_MAP_TYPE_ALLOCATED, PMM_MAP_TYPE_FREE, (uint64_t) (uintptr_t) address, (uint64_t) page_count * PMM_PAGE_SIZE)) log_panic("PMM", "Cannot free an unallocated area");
}