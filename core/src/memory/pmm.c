#include "pmm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/math.h"

size_t g_pmm_map_size;
pmm_map_entry_t g_pmm_map[PMM_MAP_MAX_ENTRIES];

static void map_insert(int index, pmm_map_entry_t entry) {
    if(g_pmm_map_size == PMM_MAP_MAX_ENTRIES) panic("memory map overflow");
    for(int i = g_pmm_map_size; i > index; i--) g_pmm_map[i] = g_pmm_map[i - 1];
    g_pmm_map[index] = entry;
    g_pmm_map_size++;
}

static void map_delete(size_t index) {
    for(size_t i = index; i < g_pmm_map_size - 1; i++) g_pmm_map[i] = g_pmm_map[i + 1];
    g_pmm_map_size--;
}

void pmm_map_set(uint64_t base, uint64_t length, pmm_map_type_t type, bool force_override) {
    if(type == PMM_MAP_TYPE_FREE || type == PMM_MAP_TYPE_ALLOCATED) {
        uint64_t aligned_base = MATH_CEIL(base, PMM_GRANULARITY);
        uint64_t base_diff = aligned_base - base;
        if(base_diff >= length) return;
        base = aligned_base;
        length = MATH_FLOOR(length - base_diff, PMM_GRANULARITY);

        if(base == 0) {
            if(length < PMM_GRANULARITY) return;
            base += PMM_GRANULARITY;
            length -= PMM_GRANULARITY;
        }
    }
    if(length == 0) return;

    for(size_t i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *map_entry = &g_pmm_map[i];

        bool override = type > map_entry->type || force_override;
        bool overlap_start = false, overlap_end = false;
        if(base > map_entry->base && base < map_entry->base + map_entry->length) overlap_start = true;
        if(base + length > map_entry->base && base + length < map_entry->base + map_entry->length) overlap_end = true;

        // Map entry falls over the entry
        if(overlap_start && overlap_end) {
            if(!override) return;

            size_t start_base = map_entry->base;
            size_t start_length = base - start_base;

            size_t end_base = base + length;
            size_t end_length = (map_entry->base + map_entry->length) - end_base;

            map_entry->base = start_base;
            map_entry->length = start_length;
            pmm_map_set(end_base, end_length, map_entry->type, true);
            continue;
        }

        // Start of entry overlaps with map entry
        if(overlap_start) {
            size_t amount = (map_entry->base + map_entry->length) - base;
            if(override) {
                map_entry->length -= amount;
                if(map_entry->length == 0) {
                    map_delete(i);
                    --i;
                }
            } else {
                length -= amount;
                base += amount;
            }
        }

        // End of entry overlaps with map entry
        if(overlap_end) {
            size_t amount = (base + length) - map_entry->base;
            if(override) {
                map_entry->length -= amount;
                map_entry->base += amount;
                if(map_entry->length == 0) {
                    map_delete(i);
                    --i;
                }
            } else {
                length -= amount;
            }
        }

        // Entry falls over map entry
        if(base < map_entry->base && base + length > map_entry->base + map_entry->length) {
            if(override) {
                map_delete(i);
                --i;
                continue;
            }

            size_t start_base = base;
            size_t start_length = map_entry->base - start_base;

            size_t end_base = map_entry->base + map_entry->length;
            size_t end_length = (base + length) - end_base;

            pmm_map_set(start_base, start_length, type, force_override);
            pmm_map_set(end_base, end_length, type, force_override);
            return;
        }

        // Entry equals map entry
        if(base == map_entry->base && length == map_entry->length) {
            if(!override) return;
            map_delete(i);
            --i;
        }
    }
    if(length == 0) return;

    size_t i = 0;
    for(; i < g_pmm_map_size && g_pmm_map[i].base <= base; i++);
    if(i != 0 && g_pmm_map[i - 1].type == type && g_pmm_map[i - 1].base + g_pmm_map[i - 1].length == base) {
        g_pmm_map[i - 1].length += length;
        return;
    }
    if(g_pmm_map[i].type == type && g_pmm_map[i].base == base + length) {
        g_pmm_map[i].base = base;
        g_pmm_map[i].length += length;
        return;
    }

    map_insert(i, (pmm_map_entry_t) {.base = base, .length = length, .type = type});
}

void *pmm_alloc(pmm_map_area_t area, size_t count) {
    size_t length = count * PMM_GRANULARITY;
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].type != PMM_MAP_TYPE_FREE) continue;
        if(g_pmm_map[i].base + g_pmm_map[i].length <= area.start || g_pmm_map[i].base >= area.end) continue;

        uint64_t ue_base = g_pmm_map[i].base;
        if(g_pmm_map[i].base < area.start) ue_base = area.start;
        uint64_t ue_length = g_pmm_map[i].length - (ue_base - g_pmm_map[i].base);
        if(ue_base + ue_length > area.end) ue_length -= (ue_base + ue_length) - area.end;
        if(ue_length < length) continue; // claim does not fit inside entry

        pmm_map_set(ue_base, length, PMM_MAP_TYPE_ALLOCATED, true);
        return (void *) (uintptr_t) ue_base;
    }
    panic("out of memory");
    __builtin_unreachable();
}

void pmm_free(void *address, size_t count) {
    pmm_map_set((uint64_t) (uintptr_t) address, count * PMM_GRANULARITY, PMM_MAP_TYPE_FREE, true);
}
