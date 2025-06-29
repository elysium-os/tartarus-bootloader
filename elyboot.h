#pragma once

#include <stdint.h>

#ifdef __TARTARUS_NO_PTR
#define ELYBOOT_PTR(TYPE) uint64_t
#else
#define ELYBOOT_PTR(TYPE) TYPE
#endif

#define ELYBOOT_KERNEL_SEGMENT_FLAG_READ (1 << 0)
#define ELYBOOT_KERNEL_SEGMENT_FLAG_WRITE (1 << 1)
#define ELYBOOT_KERNEL_SEGMENT_FLAG_EXECUTE (1 << 2)

typedef uint64_t elyboot_uint_t;

/// Physical memory map entry type
typedef enum : elyboot_uint_t {
    /// Free usable memory
    ELYBOOT_MM_TYPE_FREE,

    /// Bootloader allocated memory
    ELYBOOT_MM_TYPE_BOOTLOADER_RECLAIMABLE,
    ELYBOOT_MM_TYPE_PAGE_CACHE,

    /// Memory designated by firmware
    ELYBOOT_MM_TYPE_RESERVED,
    ELYBOOT_MM_TYPE_BAD,
    ELYBOOT_MM_TYPE_EFI_RECLAIMABLE,
    ELYBOOT_MM_TYPE_ACPI_RECLAIMABLE,
    ELYBOOT_MM_TYPE_ACPI_NVS,
} elyboot_mm_type_t;

/// Physical memory map entry
typedef struct [[gnu::packed]] {
    elyboot_mm_type_t type;
    uint64_t address;
    uint64_t length;
} elyboot_mm_entry_t;

/// Bootloader loaded module
typedef struct [[gnu::packed]] {
    ELYBOOT_PTR(const char *) name;
    uint64_t paddr;
    uint64_t size;
} elyboot_module_t;

/// Framebuffer description
typedef struct [[gnu::packed]] {
    ELYBOOT_PTR(void *) vaddr;
    uint64_t paddr;
    uint64_t size;

    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;

    struct [[gnu::packed]] {
        uint8_t red_position;
        uint8_t red_size;
        uint8_t green_position;
        uint8_t green_size;
        uint8_t blue_position;
        uint8_t blue_size;
    } mask;
} elyboot_framebuffer_t;

/// CPU initialization state
typedef enum : uint8_t {
    ELYBOOT_CPU_STATE_FAIL,
    ELYBOOT_CPU_STATE_OK
} elyboot_cpu_state_t;

/// Describes a CPU
typedef struct [[gnu::packed]] {
    uint32_t sequential_id;
    elyboot_cpu_state_t init_state;
    ELYBOOT_PTR(uint64_t *) wake_on_write;
} elyboot_cpu_t;

/// Describes a part of the loaded kernel
typedef struct [[gnu::packed]] {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t size;
    uint8_t flags;
} elyboot_kernel_segment_t;

/// Main elyboot struct
typedef struct [[gnu::packed]] {
    uint16_t version;
    uint64_t boot_timestamp;

    uint64_t acpi_rsdp;
    elyboot_uint_t entry_stack_size;

    uint64_t hhdm_base;
    uint64_t hhdm_size;

    uint64_t page_cache_base;
    uint64_t page_cache_size;

    elyboot_uint_t kernel_segment_count;
    ELYBOOT_PTR(elyboot_kernel_segment_t *) kernel_segments;

    elyboot_uint_t mm_entry_count;
    ELYBOOT_PTR(elyboot_mm_entry_t *) mm_entries;

    elyboot_uint_t bsp_index;
    elyboot_uint_t cpu_count;
    ELYBOOT_PTR(elyboot_cpu_t *) cpus;

    elyboot_uint_t module_count;
    ELYBOOT_PTR(elyboot_module_t *) modules;

    elyboot_uint_t framebuffer_count;
    ELYBOOT_PTR(elyboot_framebuffer_t *) framebuffers;
} elyboot_t;

/// Provided by the kernel to describe kernel requirements
typedef struct [[gnu::packed]] {
    uint64_t page_struct_size;
} elyboot_kernel_info_t;
