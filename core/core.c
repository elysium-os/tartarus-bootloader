#include "arch/fb.h"
#include "arch/smp.h"
#include "common/config.h"
#include "common/log.h"
#include "common/panic.h"
#include "dev/discovery.h"
#include "dev/disk.h"
#include "fs/fat.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "memory/pmm.h"
#include "protocol/protocol.h"

#include "arch/x86_64/cpu.h"

#include <stddef.h>

#define VERSION_MAJOR 4
#define VERSION_MINOR 0

#ifdef __BUILD_DEBUG
#define VERSION_TAG " (debug)"
#else
#define VERSION_TAG ""
#endif

[[noreturn]] void core() {
    log(LOG_LEVEL_INFO, "Tartarus v%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_TAG);

    for(size_t i = 0; i < g_pmm_map_size; i++) log(LOG_LEVEL_DEBUG, "pmm_map[%lu] = { base: %#llx, length: %#llx, type: %u }", i, g_pmm_map[i].base, g_pmm_map[i].length, g_pmm_map[i].type);
    log(LOG_LEVEL_INFO, "Loaded physical memory map (%lu entries)", g_pmm_map_size);

#ifdef __ARCH_X86_64
    g_smp_reserved_init_page = pmm_alloc(PMM_AREA_LOWMEM, 1);
#endif

    // Discover disks
    device_discover();
    int disk_count = 0;
    for(disk_t *disk = g_disks; disk != nullptr; disk = disk->next) disk_count++;
    log(LOG_LEVEL_INFO, "Initialized %i disks", disk_count);

    // Load config
    vfs_node_t *config_node = nullptr;
    for(disk_t *disk = g_disks; disk != nullptr; disk = disk->next) {
        for(disk_part_t *partition = disk->partitions; partition != nullptr; partition = partition->next) {
            vfs_t *fat_fs = fat_initialize(partition);
            if(fat_fs == nullptr) continue;
            vfs_node_t *node = vfs_lookup(fat_fs, "/tartarus.cfg");
            if(node == nullptr) continue;
            config_node = node;
        }
    }
    if(config_node == nullptr) panic("could not locate a config file");
    config_t *config = config_parse(config_node);
    log(LOG_LEVEL_INFO, "Config loaded (%u:%u)", config_node->vfs->partition->disk->id, config_node->vfs->partition->id);

    // Find kernel
    const char *kernel_path = config_find_string(config, "kernel", nullptr);
    if(kernel_path == nullptr) panic("no kernel path provided in config");

    vfs_node_t *kernel_node = vfs_lookup(config_node->vfs, kernel_path);
    if(kernel_node == nullptr) panic("kernel not present at \"%s\"", kernel_path);

    // Acquire framebuffer
    fb_t *fb = nullptr;
    bool retrieve_fb = config_find_bool(config, "fb", true);
    if(retrieve_fb) {
        uintmax_t fbw = config_find_number(config, "fb_width", 1920);
        uintmax_t fbh = config_find_number(config, "fb_height", 1080);
        bool strict_rgb = config_find_bool(config, "fb_strict_rgb", false);
        log(LOG_LEVEL_INFO, "Requesting framebuffer for resolution %llux%llu", fbw, fbh);

        if((fb = arch_fb_acquire(fbw, fbh, strict_rgb))) {
            log(LOG_LEVEL_INFO, "Got framebuffer with resolution %ux%u", fb->width, fb->height);
        } else {
            log(LOG_LEVEL_WARN, "Failed to acquire framebuffer");
        }
    }

    const char *protocol_name = config_find_string(config, "protocol", nullptr);
    if(protocol_name == nullptr) panic("config provides no boot protocol");

    log(LOG_LEVEL_INFO, "Using protocol: %s", protocol_name);

    protocol_t *protocol = protocol_match(protocol_name);
    if(protocol == nullptr) panic("invalid boot protocol");

    protocol->entry(config, kernel_node, fb);

    __builtin_unreachable();
}
