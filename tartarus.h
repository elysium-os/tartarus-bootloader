#ifndef __TARTARUS_BOOTLOADER_HEADER
#define __TARTARUS_BOOTLOADER_HEADER

#include <stdint.h>

#define __TARTARUS_PACKED __attribute__((packed))

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

typedef struct {
    uint64_t base;
    uint64_t length;
    tartarus_memory_map_type_t type;
} __TARTARUS_PACKED tartarus_memory_map_entry_t;

typedef struct {
bool init_failed;
__TARTARUS_PTR(uint64_t *) wake_on_write;
#ifdef __X86_64
uint8_t lapic_id;
#else
#error Missing implementation
#endif
} __TARTARUS_PACKED tartarus_cpu_t;

typedef struct {
    __TARTARUS_PTR(char *) name;
    uint64_t paddr;
    uint64_t size;
} __TARTARUS_PACKED tartarus_module_t;

typedef struct {
    uint16_t version;
    struct {
        uint64_t vaddr;
        uint64_t paddr;
        uint64_t size;
    } __TARTARUS_PACKED kernel;
    struct {
        uint64_t offset;
        uint64_t size;
    } __TARTARUS_PACKED hhdm;
    struct {
        uint16_t size;
        __TARTARUS_PTR(tartarus_memory_map_entry_t *) entries;
    } __TARTARUS_PACKED memory_map;
    struct {
        __TARTARUS_PTR(void *) address;
        uint64_t size;
        uint32_t width, height, pitch;
    } __TARTARUS_PACKED framebuffer;
    uint64_t acpi_rsdp_address;
    uint8_t bsp_index;
    uint8_t cpu_count;
    __TARTARUS_PTR(tartarus_cpu_t *) cpus;
    uint16_t module_count;
    __TARTARUS_PTR(tartarus_module_t *) modules;
    uint64_t boot_timestamp;
} __TARTARUS_PACKED tartarus_boot_info_t;

#endif