#include "arch/ptm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "memory/heap.h"
#include "memory/pmm.h"

#include "arch/x86_64/cpu.h"

#include <stddef.h>
#include <stdint.h>

#define ENTRY_FLAG_PRESENT (1 << 0)
#define ENTRY_FLAG_RW (1 << 1)
#define ENTRY_FLAG_NX ((uint64_t) 1 << 63)

#define ENTRYH_FLAG_PS (1 << 7)

#define ENTRY_4K_ADDRESS_MASK ((uint64_t) 0x0007'FFFF'FFFF'F000)
#define ENTRY_2M_ADDRESS_MASK ((uint64_t) 0x0007'FFFF'FFE0'0000)
#define ENTRY_1G_ADDRESS_MASK ((uint64_t) 0x0007'FFFF'C000'0000)

#define VADDR_TO_INDEX(VADDR, LEVEL) (((VADDR) >> ((LEVEL) * 9 + 3)) & 0x1FF)

static void map_page(ptm_address_space_t *as, uint64_t vaddr, uint64_t paddr, ptm_page_size_t page_size, bool rw, bool nx) {
    int lowest_level;
    switch(page_size) {
        case PTM_PAGE_SIZE_4K: lowest_level = 1; break;
        case PTM_PAGE_SIZE_2M: lowest_level = 2; break;
        case PTM_PAGE_SIZE_1G: lowest_level = 3; break;
    }

    uint64_t *current_table = as->top_page_table;
    for(int level = as->level_count; level > lowest_level; level--) {
        int index = VADDR_TO_INDEX(vaddr, level);

        uint64_t entry = current_table[index];
        if((entry & ENTRY_FLAG_PRESENT) == 0) {
            uint64_t *new_table = pmm_alloc(PMM_AREA_STANDARD, 1);
            memset(new_table, 0, PMM_GRANULARITY);
            entry = ENTRY_FLAG_PRESENT | ((uint64_t) (uintptr_t) new_table & ENTRY_4K_ADDRESS_MASK);
            if(nx) entry |= ENTRY_FLAG_NX;
        } else {
            if((entry & ENTRYH_FLAG_PS) != 0) panic("cannot remap over a non-4k page %#llx", entry & ENTRY_4K_ADDRESS_MASK);
            if(!nx) entry &= ~ENTRY_FLAG_NX;
        }
        if(rw) entry |= ENTRY_FLAG_RW;

        if(current_table[index] != entry) current_table[index] = entry;

        current_table = (uint64_t *) (entry & ENTRY_4K_ADDRESS_MASK);
    }

    int index = VADDR_TO_INDEX(vaddr, lowest_level);

    uint64_t address_mask;
    switch(page_size) {
        case PTM_PAGE_SIZE_4K: address_mask = ENTRY_4K_ADDRESS_MASK; break;
        case PTM_PAGE_SIZE_2M: address_mask = ENTRY_2M_ADDRESS_MASK; break;
        case PTM_PAGE_SIZE_1G: address_mask = ENTRY_1G_ADDRESS_MASK; break;
    }

    uint64_t entry = ENTRY_FLAG_PRESENT | (paddr & address_mask);
    if(page_size != PTM_PAGE_SIZE_4K) entry |= ENTRYH_FLAG_PS;
    if(rw) entry |= ENTRY_FLAG_RW;
    if(nx) entry |= ENTRY_FLAG_NX;
    current_table[index] = entry;
}

ptm_address_space_t *arch_ptm_create_address_space() {
    ptm_address_space_t *as = heap_alloc(sizeof(ptm_address_space_t));
    as->level_count = 4;

    void *top_pagemap = pmm_alloc(PMM_AREA_STANDARD, 1);
    memset(top_pagemap, 0, PMM_GRANULARITY);
    as->top_page_table = top_pagemap;

    return as;
}

void arch_ptm_map(ptm_address_space_t *as, uint64_t paddr, uint64_t vaddr, uint64_t length, uint8_t flags) {
    if(paddr % PTM_PAGE_GRANULARITY != 0 || vaddr % PTM_PAGE_GRANULARITY != 0 || length % PTM_PAGE_GRANULARITY != 0) panic("unaligned mapping (%#llx -> %#llx / %#llx)", paddr, vaddr, length);
    if((flags & PTM_FLAG_READ) == 0) log(LOG_LEVEL_WARN, "mapping with no read permission");
    if(!g_x86_64_cpu_nx_support && (flags & PTM_FLAG_EXEC) == 0) log(LOG_LEVEL_WARN, "no-execute permissions not supported");

    uint64_t offset = 0;
    while(offset < length) {
        ptm_page_size_t page_size = PTM_PAGE_SIZE_4K;
        if(paddr % PTM_PAGE_SIZE_2M == 0 && vaddr % PTM_PAGE_SIZE_2M == 0 && length - offset >= PTM_PAGE_SIZE_2M) page_size = PTM_PAGE_SIZE_2M;
        if(g_x86_64_cpu_pdpe1gb_support && paddr % PTM_PAGE_SIZE_1G == 0 && vaddr % PTM_PAGE_SIZE_1G == 0 && length - offset >= PTM_PAGE_SIZE_1G) page_size = PTM_PAGE_SIZE_1G;

        map_page(as, vaddr, paddr, page_size, (flags & PTM_FLAG_WRITE) != 0, g_x86_64_cpu_nx_support && (flags & PTM_FLAG_EXEC) == 0);
        paddr += page_size;
        vaddr += page_size;
        offset += page_size;
    }
}
