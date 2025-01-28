#include "arch/vm.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "memory/pmm.h"

#include "arch/x86_64/cpu.h"

#include <stddef.h>

#define LEVELS 4

#define PT_PRESENT (1 << 0)
#define PT_RW (1 << 1)
#define PT_LARGE (1 << 7)
#define PT_NX ((uint64_t) 1 << 63)
#define PT_ADDRESS_MASK 0x000FFFFFFFFFF000 // 4 level paging

#define PAGE_SIZE(PT_SIZE) ((PT_SIZE) == PT_SIZE_HUGE ? VM_PAGE_SIZE_1GB : ((PT_SIZE) == PT_SIZE_LARGE ? VM_PAGE_SIZE_2MB : VM_PAGE_SIZE_4KB))

typedef enum {
    PT_SIZE_NORMAL,
    PT_SIZE_LARGE,
    PT_SIZE_HUGE
} pt_size_t;

static void map_page(uint64_t *pml4, uint64_t paddr, uint64_t vaddr, pt_size_t size, bool rw, bool nx) {
    uint64_t indexes[LEVELS];
    vaddr >>= 3;
    for(int i = 0; i < LEVELS; i++) {
        vaddr >>= 9;
        indexes[LEVELS - 1 - i] = vaddr & 0x1FF;
    }

    uint64_t *current_table = pml4;
    int highest_index = LEVELS - (size == PT_SIZE_HUGE ? 3 : (size == PT_SIZE_LARGE ? 2 : 1));
    for(int i = 0; i < highest_index; i++) {
        if((current_table[indexes[i]] & PT_PRESENT) == 0) {
            uint64_t *new_table = pmm_alloc(PMM_AREA_STANDARD, 1);
            memset(new_table, 0, PMM_GRANULARITY);
            current_table[indexes[i]] = ((uint64_t) (uintptr_t) new_table & PT_ADDRESS_MASK) | PT_PRESENT;
        }
        if(rw) current_table[indexes[i]] |= PT_RW;
        if(!nx) current_table[indexes[i]] &= ~PT_NX;
        current_table = (uint64_t *) (uintptr_t) (current_table[indexes[i]] & PT_ADDRESS_MASK);
    }
    current_table[indexes[highest_index]] = (paddr & PT_ADDRESS_MASK) | PT_PRESENT;
    if(size != PT_SIZE_NORMAL) current_table[indexes[highest_index]] |= PT_LARGE;
    if(rw) current_table[indexes[highest_index]] |= PT_RW;
    if(nx) current_table[indexes[highest_index]] |= PT_NX;
}

void *arch_vm_create_address_space() {
    void *pml4 = pmm_alloc(PMM_AREA_STANDARD, 1);
    memset(pml4, 0, PMM_GRANULARITY);
    return pml4;
}

void arch_vm_load_address_space(void *address_space) {
    asm volatile("mov %0, %%cr3" : : "r"(address_space) : "memory");
}

void arch_vm_map(void *address_space, uint64_t paddr, uint64_t vaddr, uint64_t length, uint8_t flags) {
    if(paddr % VM_PAGE_GRANULARITY != 0 || vaddr % VM_PAGE_GRANULARITY != 0 || length % VM_PAGE_GRANULARITY != 0) panic("unaligned mapping (%#llx -> %#llx / %#llx)", paddr, vaddr, length);
    if((flags & VM_FLAG_READ) == 0) log(LOG_LEVEL_WARN, "mapping with no read permission");

    uint64_t offset = 0;
    while(offset < length) {
        pt_size_t pt_size = PT_SIZE_NORMAL;
        if(paddr % PAGE_SIZE(PT_SIZE_LARGE) == 0 && vaddr % PAGE_SIZE(PT_SIZE_LARGE) == 0 && length - offset >= PAGE_SIZE(PT_SIZE_LARGE)) pt_size = PT_SIZE_LARGE;
        if(paddr % PAGE_SIZE(PT_SIZE_HUGE) == 0 && vaddr % PAGE_SIZE(PT_SIZE_HUGE) == 0 && length - offset >= PAGE_SIZE(PT_SIZE_HUGE)) pt_size = PT_SIZE_HUGE;

        map_page(address_space, paddr, vaddr, pt_size, (flags & VM_FLAG_WRITE) != 0, g_x86_64_cpu_nx_support && (flags & VM_FLAG_EXEC) == 0);
        paddr += PAGE_SIZE(pt_size);
        vaddr += PAGE_SIZE(pt_size);
        offset += PAGE_SIZE(pt_size);
    }
}
