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
#define __TARTARUS_PTR(TYPE) tartarus_vaddr_t
#else
#define __TARTARUS_PTR(TYPE) TYPE
#endif

#define TARTARUS_KERNEL_SEGMENT_FLAG_READ (1 << 0)
#define TARTARUS_KERNEL_SEGMENT_FLAG_WRITE (1 << 1)
#define TARTARUS_KERNEL_SEGMENT_FLAG_EXECUTE (1 << 2)

typedef uint64_t tartarus_paddr_t;
typedef uint64_t tartarus_vaddr_t;
typedef uint64_t tartarus_size_t;

/// Physical memory map entry type
typedef enum : uint64_t {
    /// Usable and free memory
    TARTARUS_MM_TYPE_USABLE,

    /// Usable memory used by the bootloader
    TARTARUS_MM_TYPE_BOOTLOADER_RECLAIMABLE,

    /// Usable memory used by EFI
    TARTARUS_MM_TYPE_EFI_RECLAIMABLE,

    /// Usable memory used by ACPI
    TARTARUS_MM_TYPE_ACPI_RECLAIMABLE,

    /// ACPI NVS memory
    TARTARUS_MM_TYPE_ACPI_NVS,

    /// Reserved by firmware
    TARTARUS_MM_TYPE_RESERVED,

    /// Memory marked as bad by firmware
    TARTARUS_MM_TYPE_BAD
} tartarus_mm_type_t;

/// Physical memory map entry
typedef struct [[gnu::packed]] {
    tartarus_mm_type_t type;
    tartarus_paddr_t base;
    tartarus_size_t length;
} tartarus_mm_entry_t;

/// CPU initialization state
typedef enum : uint8_t {
    /// Tartarus failed to initialize the CPU
    TARTARUS_CPU_STATE_FAIL,

    /// The CPU successfully initialized and is parked
    TARTARUS_CPU_STATE_OK
} tartarus_cpu_state_t;

/// Describes a CPU
typedef struct [[gnu::packed]] {
    tartarus_cpu_state_t init_state;
    uint64_t sequential_id;
    __TARTARUS_PTR(tartarus_vaddr_t *) wake_on_write;
} tartarus_cpu_t;

/// Module loaded by Tartarus
typedef struct [[gnu::packed]] {
    __TARTARUS_PTR(char *) name;
    tartarus_paddr_t paddr;
    tartarus_size_t size;
} tartarus_module_t;

// Framebuffer initialized by Tartarus
typedef struct [[gnu::packed]] {
    __TARTARUS_PTR(void *) vaddr;
    tartarus_paddr_t paddr;
    tartarus_size_t size;

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

/// Describes a loaded segment of the kernel
typedef struct [[gnu::packed]] {
    tartarus_vaddr_t vaddr;
    tartarus_paddr_t paddr;
    tartarus_size_t size;
    uint8_t flags;
} tartarus_kernel_segment_t;

/// Main boot information
typedef struct [[gnu::packed]] {
    uint16_t version;
    uint64_t boot_timestamp;

    tartarus_paddr_t acpi_rsdp_address;
    tartarus_size_t bsp_entry_stack_size;
    tartarus_size_t ap_entry_stack_size;

    tartarus_vaddr_t hhdm_offset;
    tartarus_size_t hhdm_size;

    tartarus_size_t kernel_segment_count;
    __TARTARUS_PTR(tartarus_kernel_segment_t *) kernel_segments;

    tartarus_size_t mm_entry_count;
    __TARTARUS_PTR(tartarus_mm_entry_t *) mm_entries;

    tartarus_size_t framebuffer_count;
    __TARTARUS_PTR(tartarus_framebuffer_t *) framebuffers;

    tartarus_size_t module_count;
    __TARTARUS_PTR(tartarus_module_t *) modules;

    tartarus_size_t bsp_index;
    tartarus_size_t cpu_count;
    __TARTARUS_PTR(tartarus_cpu_t *) cpus;
} tartarus_boot_info_t;

#endif
