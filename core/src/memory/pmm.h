#pragma once
#include <stddef.h>
#include <stdint.h>

#define PMM_MAX_MAP_ENTRIES 512

#ifdef __X86_64
#define PMM_PAGE_SIZE 0x1000
#define PMM_PAGE_SIZE_LARGE 0x200000

#define PMM_AREA_CONVENTIONAL ((pmm_map_area_t) { .start = 0, .end = 0xA0000 })
#define PMM_AREA_LOWMEM ((pmm_map_area_t) { .start = 0xA0000, .end = 0x100000 })
#define PMM_AREA_STANDARD ((pmm_map_area_t) { .start = 0x100000, .end = UINTPTR_MAX })
#else
#error Invalid target or missing implementation
#endif

typedef enum {
    PMM_MAP_TYPE_FREE,
    PMM_MAP_TYPE_ALLOCATED,
    PMM_MAP_TYPE_ACPI_RECLAIMABLE,
    PMM_MAP_TYPE_ACPI_NVS,
    PMM_MAP_TYPE_RESERVED,
    PMM_MAP_TYPE_BAD
} pmm_map_type_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    pmm_map_type_t type;
} pmm_map_entry_t;

typedef struct {
    uint64_t start, end;
} pmm_map_area_t;

extern uint16_t g_pmm_map_size;
extern pmm_map_entry_t g_pmm_map[PMM_MAX_MAP_ENTRIES];

void pmm_init_add(uint64_t base, uint64_t length, pmm_map_type_t type);

/*
 *  Converts memory from an entry type to another type
 *  Returns true if the region given is not governed by the src type (This also means it did not perform the conversion).
 */
bool pmm_convert(pmm_map_type_t src_type, pmm_map_type_t dest_type, uint64_t base, uint64_t length);

void *pmm_alloc_ext(pmm_map_area_t area, pmm_map_type_t src_type, size_t page_count);
void *pmm_alloc(pmm_map_area_t area, size_t page_count);
void pmm_free(void *address, size_t page_count);