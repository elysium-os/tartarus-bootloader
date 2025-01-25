#include "arch/disk.h"
#include "common/config.h"
#include "common/log.h"
#include "common/panic.h"
#include "dev/disk.h"
#include "fs/fat.h"
#include "fs/vfs.h"
#include "memory/pmm.h"

#include <stddef.h>

#define VERSION_MAJOR 4
#define VERSION_MINOR 0

#ifdef __ENV_DEVELOPMENT
#define VERSION_TAG " (development)"
#else
#define VERSION_TAG ""
#endif

[[noreturn]] void core() {
    log(LOG_LEVEL_INFO, "Tartarus v%u.%u%s", VERSION_MAJOR, VERSION_MINOR, VERSION_TAG);

    for(size_t i = 0; i < g_pmm_map_size; i++) log(LOG_LEVEL_DEBUG, "PMM_MAP[%lu] = { base: %#llx, length: %#llx, type: %u }", i, g_pmm_map[i].base, g_pmm_map[i].length, g_pmm_map[i].type);
    log(LOG_LEVEL_INFO, "Loaded physical memory map (%lu entries)", g_pmm_map_size);

    // Initialize disks
    arch_disk_initialize();
    int disk_count = 0;
    for(disk_t *disk = g_disks; disk != NULL; disk = disk->next) disk_count++;
    log(LOG_LEVEL_INFO, "Initialized %i disks", disk_count);

    // Load config
    vfs_node_t *config_node = NULL;
    for(disk_t *disk = g_disks; disk != NULL; disk = disk->next) {
        for(disk_part_t *partition = disk->partitions; partition != NULL; partition = partition->next) {
            vfs_t *fat_fs = fat_initialize(partition);
            if(fat_fs == NULL) continue;
            vfs_node_t *node = vfs_lookup(fat_fs, "/tartarus.cfg");
            if(node == NULL) continue;
            config_node = node;
        }
    }
    if(config_node == NULL) panic("could not locate a config file");
    config_t *config = config_parse(config_node);
    log(LOG_LEVEL_INFO, "Config loaded (%u:%u)", config_node->vfs->partition->disk->id, config_node->vfs->partition->id);

    // Find kernel
    const char *kernel_path;
    if(!config_find_string(config, "kernel", &kernel_path)) panic("no kernel path provided in config");
    vfs_node_t *kernel_node = vfs_lookup(config_node->vfs, kernel_path);
    if(kernel_node == NULL) panic("kernel not present at \"%s\"", kernel_path);

    for(;;);
}
