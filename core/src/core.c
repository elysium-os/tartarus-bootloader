#include "core.h"
#include <stdint.h>
#include <common/log.h>
#include <common/config.h>
#include <common/fb.h>
#include <lib/str.h>
#include <drivers/disk.h>
#include <fs/vfs.h>
#include <fs/fat.h>
#include <protocol/protocol.h>

#define REGISTER_PROTOCOL(PROTOCOL) if(strcmp(protocol, #PROTOCOL) == 0) { protocol_##PROTOCOL(config, kernel_node, fb); __builtin_unreachable(); }

[[noreturn]] void core() {
    // Initialize disk
    disk_initialize();
    int disk_count = 0;
    for(disk_t *disk = g_disks; disk != NULL; disk = disk->next) disk_count++;
    log("CORE", "Initialized disks (%i disks)", disk_count);

    // Locate config
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
    if(config_node == NULL) log_panic("CORE", "Could not locate a config file");
    config_t *config = config_parse(config_node);
    log("CORE", "Config loaded");

    // Find kernel
    char *kernel_path = config_read_string(config, "KERNEL");
    if(kernel_path == NULL) log_panic("CORE", "No kernel path provided in config");
    vfs_node_t *kernel_node = vfs_lookup(config_node->vfs, kernel_path);
    if(kernel_node == NULL) log_panic("CORE", "Kernel not present at \"%s\"", kernel_path);

    // Framebuffer
    int scrw = config_read_int(config, "SCRW", 1920);
    int scrh = config_read_int(config, "SCRH", 1080);
    log("CORE", "Screen size is presumed to be %ix%i", scrw, scrh);

    fb_t fb;
    if(fb_acquire(scrw, scrh, false, &fb)) log_panic("CORE", "Failed to retrieve framebuffer");
    log("CORE", "Acquired framebuffer (%ux%u)", fb.width, fb.height);

    // Protocol
    char *protocol = config_read_string(config, "PROTOCOL");
    if(protocol == NULL) log_panic("CORE", "No protocol defined in config");
    log("CORE", "Protocol is %s", protocol);

    REGISTER_PROTOCOL(linux);
    REGISTER_PROTOCOL(tartarus);

    config_free(config);
    log_panic("CORE", "Invalid protocol %s", protocol);
    __builtin_unreachable();
}