#pragma once

#include <stddef.h>
#include <stdint.h>

#define PTM_FLAG_EXEC (1 << 0)
#define PTM_FLAG_WRITE (1 << 1)
#define PTM_FLAG_READ (1 << 2)

#define PTM_PAGE_GRANULARITY PTM_PAGE_SIZE_4K

typedef enum : uint64_t {
    PTM_PAGE_SIZE_4K = 0x1000,
    PTM_PAGE_SIZE_2M = 0x200000,
    PTM_PAGE_SIZE_1G = 0x40000000
} ptm_page_size_t;

#ifdef __ARCH_X86_64

#define PTM_VA_BITS(ADDRESS_SPACE) ((ADDRESS_SPACE)->level_count == 4 ? 48 : 52)

typedef struct {
    size_t level_count;
    uint64_t *top_page_table;
} ptm_address_space_t;

#elif __ARCH_AARCH64

#define PTM_VA_BITS(ADDRESS_SPACE) ((ADDRESS_SPACE)->level_count == 4 ? 48 : 57)

typedef struct {
    size_t level_count;
    uint64_t *top_page_tables[2];
} ptm_address_space_t;

#else
#error Unimplemented
#endif

ptm_address_space_t *arch_ptm_create_address_space();
void arch_ptm_map(ptm_address_space_t *address_space, uint64_t paddr, uint64_t vaddr, uint64_t length, uint8_t flags);
void arch_ptm_load_address_space(ptm_address_space_t *address_space);
