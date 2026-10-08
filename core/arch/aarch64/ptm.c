#include "arch/ptm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include <stddef.h>
#include <stdint.h>

#define LEVELS 4

#define PT_VALID (1 << 0)
#define PT_TABLE (1 << 1)
#define PT_LL3_PAGE (1 << 1)
#define PT_READ_ONLY (1 << 7)
#define PT_ACCESS (1 << 10)
#define PT_INNER_SHAREABLE (0b11 << 8)
#define PT_EXEC_NEVER ((uint64_t) 1 << 54)
#define PT_ADDRESS_MASK 0x0000FFFFFFFFF000

#define VADDR_TO_INDEX(VADDR, LEVEL) (((VADDR) >> ((LEVEL) * 9 + 3)) & 0x1FF)

#define TABLE_FLAGS (PT_VALID | PT_TABLE)
#define PAGE_FLAGS (PT_VALID | PT_ACCESS | PT_INNER_SHAREABLE)

static void map_page(ptm_address_space_t *address_space, uint64_t paddr, uint64_t vaddr, ptm_page_size_t page_size, bool readonly, bool exec_never) {
    uint64_t page_flags = PAGE_FLAGS;
    if(readonly) page_flags |= PT_READ_ONLY;
    if(exec_never) page_flags |= PT_EXEC_NEVER;

    int lowest_index;
    switch(page_size) {
        case PTM_PAGE_SIZE_4K: lowest_index = 1; break;
        case PTM_PAGE_SIZE_2M: lowest_index = 2; break;
        case PTM_PAGE_SIZE_1G: lowest_index = 3; break;
    }

    bool is_higher_half = (vaddr & (1llu << 63)) != 0;

    uint64_t *table = address_space->top_page_tables[is_higher_half ? 1 : 0];
    for(int level = address_space->level_count; level > lowest_index; level--) {
        uint64_t index = VADDR_TO_INDEX(vaddr, level);

        if((table[index] & PT_VALID) == 0) {
            uint64_t *new_table = pmm_alloc(PMM_AREA_STANDARD, 1);
            memset(new_table, 0, PMM_GRANULARITY);
            table[index] = ((uintptr_t) new_table & PT_ADDRESS_MASK) | TABLE_FLAGS;

            table = new_table;
            continue;
        }

        uintptr_t entry_paddr = table[index] & PT_ADDRESS_MASK;
        if((table[index] & PT_TABLE) == 0) panic("cannot remap over a non-4k page");

        table = (uint64_t *) entry_paddr;
    }


    uint64_t entry = paddr | page_flags;
    if(page_size == PTM_PAGE_SIZE_4K) entry |= PT_LL3_PAGE;
    table[VADDR_TO_INDEX(vaddr, lowest_index)] = entry;
}

ptm_address_space_t *arch_ptm_create_address_space() {
    ptm_address_space_t *as = heap_alloc(sizeof(ptm_address_space_t));
    as->level_count = 4;
    for(size_t i = 0; i < sizeof(as->top_page_tables) / sizeof(as->top_page_tables[0]); i++) {
        as->top_page_tables[i] = pmm_alloc(PMM_AREA_STANDARD, 1);
        memset(as->top_page_tables[i], 0, PMM_GRANULARITY);
    }
    return as;
}

void arch_ptm_map(ptm_address_space_t *address_space, uint64_t paddr, uint64_t vaddr, uint64_t length, uint8_t flags) {
    if(paddr % PTM_PAGE_GRANULARITY != 0 || vaddr % PTM_PAGE_GRANULARITY != 0 || length % PTM_PAGE_GRANULARITY != 0) panic("unaligned mapping (%#llx -> %#llx / %#llx)", paddr, vaddr, length);
    if((flags & PTM_FLAG_READ) == 0) log(LOG_LEVEL_WARN, "mapping with no read permission");

    uint64_t offset = 0;
    while(offset < length) {
        ptm_page_size_t page_size = PTM_PAGE_SIZE_4K;
        if(paddr % PTM_PAGE_SIZE_2M == 0 && vaddr % PTM_PAGE_SIZE_2M == 0 && length - offset >= PTM_PAGE_SIZE_2M) page_size = PTM_PAGE_SIZE_2M;
        if(paddr % PTM_PAGE_SIZE_1G == 0 && vaddr % PTM_PAGE_SIZE_1G == 0 && length - offset >= PTM_PAGE_SIZE_1G) page_size = PTM_PAGE_SIZE_1G;

        map_page(address_space, paddr, vaddr, page_size, (flags & PTM_FLAG_WRITE) == 0, (flags & PTM_FLAG_EXEC) == 0);
        paddr += page_size;
        vaddr += page_size;
        offset += page_size;
    }
}
