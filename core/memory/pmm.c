#include "pmm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/math.h"

#define SANITIZE_TYPE(TYPE) ((TYPE) == PMM_MAP_TYPE_FREE || (TYPE) == PMM_MAP_TYPE_ALLOCATED || (TYPE) == PMM_MAP_TYPE_EFI_RECLAIMABLE || (TYPE) == PMM_MAP_TYPE_ACPI_RECLAIMABLE)

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

static void map_insert_region(uint64_t base, uint64_t length, pmm_map_type_t type) {
    size_t i = 0;
    for(; i < g_pmm_map_size && g_pmm_map[i].base <= base; i++);

    if(i != 0) {
        pmm_map_entry_t *previous = &g_pmm_map[i - 1];
        if(previous->type == type && previous->base + previous->length == base) {
            previous->length += length;
            return;
        }
    }

    if(i < g_pmm_map_size) {
        pmm_map_entry_t *next = &g_pmm_map[i];
        if(next->type == type && next->base == base + length) {
            next->base = base;
            next->length += length;
            return;
        }
    }

    map_insert(i, (pmm_map_entry_t) {.base = base, .length = length, .type = type});
}

void pmm_map_set(uint64_t base, uint64_t length, pmm_map_type_t type, bool force) {
    if(length == 0) return;
    uint64_t end = base + length;

    // Remove / trim existing entries that we override
    for(size_t i = 0; i < g_pmm_map_size;) {
        pmm_map_entry_t *entry = &g_pmm_map[i];
        uint64_t entry_end = entry->base + entry->length;

        if(entry->base >= end || entry_end <= base) {
            i++;
            continue;
        }

        if(!(type > entry->type || force)) {
            i++;
            continue;
        }

        if(base <= entry->base && end >= entry_end) {
            // existing entry is completely covered
            map_delete(i);
        } else if(entry->base < base && entry_end > end) {
            // new region splits the existing entry in two
            uint64_t old_end = entry_end;
            entry->length = base - entry->base;
            map_insert(i + 1, (pmm_map_entry_t) {.base = end, .length = old_end - end, .type = entry->type});
            i += 2;
        } else if(entry->base < base) {
            // existing entry overlaps the start of the new region
            entry->length = base - entry->base;
            i++;
        } else {
            // existing entry overlaps the end of the new region
            entry->base = end;
            entry->length = entry_end - end;
            i++;
        }
    }

    // insert the parts of the new region not already occupied by higher priority entries
    // the new region may be split into multiple pieces
    uint64_t current = base;
    while(current < end) {
        uint64_t stop = end;
        bool blocked = false;

        for(size_t i = 0; i < g_pmm_map_size; i++) {
            pmm_map_entry_t *entry = &g_pmm_map[i];
            uint64_t entry_end = entry->base + entry->length;

            if(entry_end <= current) continue;
            if(entry->base >= end) {
                if(entry->base < stop) stop = entry->base;
                break;
            }
            if(entry->base > current) {
                stop = entry->base;
                break;
            }

            current = entry_end;
            blocked = true;
            break;
        }

        if(blocked) continue;
        if(stop > current) {
            map_insert_region(current, stop - current, type);
            current = stop;
        } else {
            break;
        }
    }
}

void pmm_map_add(uint64_t base, uint64_t length, pmm_map_type_t type) {
    if(SANITIZE_TYPE(type)) {
        uint64_t difference = PMM_GRANULARITY - (base % PMM_GRANULARITY);
        if(difference >= length) return;

        base += difference;
        length = MATH_FLOOR(length - difference, PMM_GRANULARITY);

        if(base == 0) {
            if(length < PMM_GRANULARITY) return;
            base += PMM_GRANULARITY;
            length -= PMM_GRANULARITY;
        }
    }
    if(length == 0) return;

    pmm_map_set(base, length, type, false);
}

bool pmm_alloc_at(uint64_t address, size_t page_count, pmm_map_type_t type) {
    size_t length = page_count * PMM_GRANULARITY;
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].type != PMM_MAP_TYPE_FREE) continue;
        if(g_pmm_map[i].base > address || g_pmm_map[i].base + g_pmm_map[i].length < address + length) continue;

        pmm_map_set(address, length, type, true);
        return true;
    }
    return false;
}

void *pmm_alloc_ext(pmm_map_area_t area, size_t page_count, size_t alignment, pmm_map_type_t type) {
    size_t length = page_count * PMM_GRANULARITY;
    for(size_t i = 0; i < g_pmm_map_size; i++) {
        if(g_pmm_map[i].type != PMM_MAP_TYPE_FREE) continue;
        if(g_pmm_map[i].base + g_pmm_map[i].length <= area.start || g_pmm_map[i].base >= area.end) continue;

        uint64_t ue_base = MATH_CEIL(g_pmm_map[i].base, alignment);
        if(ue_base >= g_pmm_map[i].base + g_pmm_map[i].length) continue;
        if(g_pmm_map[i].base < area.start) ue_base = area.start;
        uint64_t ue_length = g_pmm_map[i].length - (ue_base - g_pmm_map[i].base);
        if(ue_base + ue_length > area.end) ue_length -= (ue_base + ue_length) - area.end;
        if(ue_length < length) continue; // claim does not fit inside entry

        pmm_map_set(ue_base, length, type, true);
        return (void *) (uintptr_t) ue_base;
    }
    panic("out of memory");
    __builtin_unreachable();
}

void *pmm_alloc(pmm_map_area_t area, size_t page_count) {
    return pmm_alloc_ext(area, page_count, PMM_GRANULARITY, PMM_MAP_TYPE_ALLOCATED);
}

void pmm_free(void *address, size_t page_count) {
    pmm_map_set((uint64_t) (uintptr_t) address, page_count * PMM_GRANULARITY, PMM_MAP_TYPE_FREE, true);
}
