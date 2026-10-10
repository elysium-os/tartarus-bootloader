#include "arch/ptm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"
#include "memory/pmm.h"
#include "smoldtb.h"

#include "arch/riscv64/csr.h"

#include <stddef.h>
#include <stdint.h>

#define PTE_V ((uint64_t) (1ULL << 0))
#define PTE_R ((uint64_t) (1ULL << 1))
#define PTE_W ((uint64_t) (1ULL << 2))
#define PTE_X ((uint64_t) (1ULL << 3))
#define PTE_G ((uint64_t) (1ULL << 5))
#define PTE_PPN_SHIFT 10
#define PTE_PPN_MASK ((uint64_t) (((uint64_t) 1 << 44) - 1))

#define PA_TO_PPN(pa) (((uint64_t) (uintptr_t) (pa) >> 12) & PTE_PPN_MASK)
#define PTE_TO_PA(pte) ((uintptr_t) (((pte) >> PTE_PPN_SHIFT) << 12))
#define VADDR_TO_INDEX(vaddr, level) ((unsigned int) (((uint64_t) (vaddr) >> (12 + 9 * ((int) (level) - 1))) & 0x1FFUL))

#define PAGE_SIZE_AT_LEVEL(level) ((uint64_t) PTM_PAGE_SIZE_4K << (9 * ((int) (level) - 1)))

static size_t detect_max_level_count() {
    // @todo: acpi support
    dtb_node *cpus = dtb_find("/cpus");
    if(cpus == nullptr) {
        panic("fdt: /cpus node not found\n");
        return 0;
    }

    for(dtb_node *node = dtb_get_child(cpus); node; node = dtb_get_sibling(node)) {
        dtb_prop *device_type = dtb_find_prop(node, "device_type");
        if(!device_type) continue;

        const char *type = dtb_read_prop_string(device_type, 0);
        if(!string_eq(type, "cpu")) continue;

        dtb_prop *status_prop = dtb_find_prop(node, "status");
        if(!status_prop) continue;

        const char *status = dtb_read_prop_string(status_prop, 0);
        if(!string_eq(status, "okay") && !string_eq(status, "ok")) { continue; }

        dtb_prop *mmu_prop = dtb_find_prop(node, "mmu-type");
        if(!mmu_prop) continue;

        const char *mmu = dtb_read_prop_string(mmu_prop, 0);

        if(mmu) {
            if(string_eq(mmu, "riscv,sv57")) {
                log(LOG_LEVEL_DEBUG, "ptm: mmu-type = \"%s\"\n", mmu);
                return 5;
            }
            if(string_eq(mmu, "riscv,sv48")) {
                log(LOG_LEVEL_DEBUG, "ptm: mmu-type = \"%s\"\n", mmu);
                return 4;
            }
            if(string_eq(mmu, "riscv,sv39")) {
                log(LOG_LEVEL_DEBUG, "ptm: mmu-type = \"%s\"\n", mmu);
                return 3;
            }
            log(LOG_LEVEL_DEBUG, "fdt: unknown mmu-type \"%s\"\n", mmu);
            panic("unsupported mmu type");
        }

        break;
    }

    log(LOG_LEVEL_WARN, "could not determine mmu level count from device tree\n");
    // @todo: fall back to acpi if not found
    panic("unimplemented...");
    return 0;
}

ptm_address_space_t *arch_ptm_create_address_space() {
    ptm_address_space_t *as = heap_alloc(sizeof(ptm_address_space_t));
    as->level_count = detect_max_level_count();

    void *top_pagemap = pmm_alloc(PMM_AREA_STANDARD, 1);
    memset(top_pagemap, 0, PMM_GRANULARITY);
    as->top_page_table = top_pagemap;

    uint64_t mode = 0;
    switch(as->level_count) {
        case 3: mode = ARCH_CSR_SATP_MODE_SV39; break;
        case 4: mode = ARCH_CSR_SATP_MODE_SV48; break;
        case 5: mode = ARCH_CSR_SATP_MODE_SV57; break;
    }

    as->satp_value = ARCH_CSR_SATP_MAKE(mode, (uintptr_t) as->top_page_table);

    return as;
}

static void map_page(ptm_address_space_t *address_space, uint64_t paddr, uint64_t vaddr, ptm_page_size_t page_size, bool readonly, bool exec_never) {
    uint64_t leaf_flags = PTE_V | PTE_R;
    if(!readonly) leaf_flags |= PTE_W;
    if(!exec_never) leaf_flags |= PTE_X;

    int lowest_index;
    switch(page_size) {
        case PTM_PAGE_SIZE_4K: lowest_index = 1; break;
        case PTM_PAGE_SIZE_2M: lowest_index = 2; break;
        case PTM_PAGE_SIZE_1G: lowest_index = 3; break;
    }

    uint64_t *table = (uint64_t *) (address_space->top_page_table);

    for(int level = address_space->level_count; level > lowest_index; level--) {
        uint64_t index = VADDR_TO_INDEX(vaddr, level);
        uint64_t entry = table[index];

        if((table[index] & PTE_V) == 0) {
            uint64_t *new_table = pmm_alloc(PMM_AREA_STANDARD, 1);
            memset(new_table, 0, PMM_GRANULARITY);
            table[index] = (PA_TO_PPN(new_table) << PTE_PPN_SHIFT) | PTE_V;

            table = new_table;
            continue;
        } else if(entry & (PTE_R | PTE_W | PTE_X)) {
            panic("ptm: walked into existing leaf PTE at level %d (vaddr=0x%lx)", level, vaddr);
        }

        table = (uint64_t *) (PTE_TO_PA(entry));
    }

    uint64_t idx = VADDR_TO_INDEX(vaddr, lowest_index);
    table[idx] = (PA_TO_PPN(paddr) << PTE_PPN_SHIFT) | leaf_flags;
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
