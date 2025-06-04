#ifndef __TARTARUS_BOOTLOADER_HEADER
#define __TARTARUS_BOOTLOADER_HEADER

#include <stdint.h>

#ifndef __TARTARUS_ARCH_OVERRIDE
#if defined __x86_64__ || defined __i386__ || defined _M_X64 || defined _M_IX86
#ifndef __X86_64
#define __X86_64
#endif
#endif
#endif

#ifdef __TARTARUS_NO_PTR
#define __TARTARUS_PTR(TYPE) uint64_t
#else
#define __TARTARUS_PTR(TYPE) TYPE
#endif

typedef enum {
    TARTARUS_MEMORY_MAP_TYPE_USABLE,
    TARTARUS_MEMORY_MAP_TYPE_BOOT_RECLAIMABLE,
    TARTARUS_MEMORY_MAP_TYPE_ACPI_RECLAIMABLE,
    TARTARUS_MEMORY_MAP_TYPE_ACPI_NVS,
    TARTARUS_MEMORY_MAP_TYPE_RESERVED,
    TARTARUS_MEMORY_MAP_TYPE_BAD
} tartarus_memory_map_type_t;

typedef struct [[gnu::packed]] {
    uint64_t base;
    uint64_t length;
    tartarus_memory_map_type_t type;
} tartarus_memory_map_entry_t;

typedef struct [[gnu::packed]] {
    bool initialization_failed;
    __TARTARUS_PTR(uint64_t *) wake_on_write;
#ifdef __X86_64
    uint8_t lapic_id;
#else
#error Missing implementation
#endif
} tartarus_cpu_t;

typedef struct [[gnu::packed]] {
    __TARTARUS_PTR(char *) name;
    uint64_t paddr;
    uint64_t size;
} tartarus_module_t;

typedef struct [[gnu::packed]] {
    __TARTARUS_PTR(void *) address;
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
} tartarus_framebuffer_t;

typedef struct [[gnu::packed]] {
    uint64_t vaddr;
    uint64_t size;
    bool read, write, execute;
} tartarus_kernel_segment_t;

typedef struct [[gnu::packed]] {
    uint16_t version;

    struct [[gnu::packed]] {
        uint64_t vaddr;
        uint64_t paddr;
        uint64_t size;
        uint64_t segment_count;
        __TARTARUS_PTR(tartarus_kernel_segment_t *) segments;
    } kernel;
    struct [[gnu::packed]] {
        uint64_t offset;
        uint64_t size;
    } hhdm;
    struct [[gnu::packed]] {
        uint16_t size;
        __TARTARUS_PTR(tartarus_memory_map_entry_t *) entries;
    } memory_map;
    uint64_t acpi_rsdp_address;

    uint64_t framebuffer_count;
    __TARTARUS_PTR(tartarus_framebuffer_t *) framebuffers;

    uint8_t bsp_index;
    uint8_t cpu_count;
    __TARTARUS_PTR(tartarus_cpu_t *) cpus;

    uint16_t module_count;
    __TARTARUS_PTR(tartarus_module_t *) modules;

    uint64_t boot_timestamp;
} tartarus_boot_info_t;

#endif
