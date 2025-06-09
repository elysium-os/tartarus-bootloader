#pragma once

#include <stdint.h>

#define VM_FLAG_EXEC (1 << 0)
#define VM_FLAG_WRITE (1 << 1)
#define VM_FLAG_READ (1 << 2)

#ifdef __ARCH_X86_64
#define VM_PAGE_SIZE_1GB 0x4000'0000
#define VM_PAGE_SIZE_2MB 0x20'0000
#define VM_PAGE_SIZE_4KB 0x1000
#define VM_PAGE_GRANULARITY VM_PAGE_SIZE_4KB
#endif

void *arch_vm_create_address_space();
void arch_vm_load_address_space(void *address_space);
void arch_vm_map(void *address_space, uint64_t paddr, uint64_t vaddr, uint64_t length, uint8_t flags);
