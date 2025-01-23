#include "pmm.h"

#include "common/panic.h"

size_t g_pmm_map_size;
pmm_map_entry_t g_pmm_map[PMM_MAP_MAX_ENTRIES];

static void map_insert(int index, pmm_map_entry_t entry) {
    if(g_pmm_map_size == PMM_MAP_MAX_ENTRIES) panic("PMM", "Memory map overflow");
    for(int i = g_pmm_map_size; i > index; i--) g_pmm_map[i] = g_pmm_map[i - 1];
    g_pmm_map[index] = entry;
    g_pmm_map_size++;
}

static size_t map_add_sorted(uint64_t base, uint64_t length, pmm_map_type_t type) {
    size_t i = 0;
    for(; i < g_pmm_map_size && g_pmm_map[i].base <= base; i++);
    map_insert(i, (pmm_map_entry_t) {.base = base, .length = length, .type = type});
    return i;
}

static void map_delete(size_t index) {
    for(size_t i = index; i < g_pmm_map_size - 1; i++) g_pmm_map[i] = g_pmm_map[i + 1];
    g_pmm_map_size--;
}

void pmm_map_add(uint64_t base, uint64_t length, pmm_map_type_t type) {
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        pmm_map_entry_t *map_entry = &g_pmm_map[i];

        bool override = type > map_entry->type;
        bool overlap_start = false, overlap_end = false;
        if(base >= map_entry->base && base < map_entry->base + map_entry->length) overlap_start = true;
        if(base + length > map_entry->base && base + length <= map_entry->base + map_entry->length) overlap_end = true;

        // Map entry falls over the entry
        if(overlap_start && overlap_end) {
            if(!override) return;

            size_t start_base = map_entry->base;
            size_t start_length = base - start_base;

            size_t end_base = base + length;
            size_t end_length = (map_entry->base + map_entry->length) - end_base;

            map_entry->base = start_base;
            map_entry->length = start_length;
            map_add_sorted(end_base, end_length, map_entry->type);
            continue;
        }

        // Start of entry overlaps with map entry
        if(overlap_start) {
            size_t amount = (map_entry->base + map_entry->length) - base;
            if(override) {
                map_entry->length -= amount;
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

            pmm_map_add(start_base, start_length, type);
            pmm_map_add(end_base, end_length, type);
            return;
        }
    }
    map_add_sorted(base, length, type);
}
