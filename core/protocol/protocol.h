#pragma once

#include "arch/fb.h"
#include "fs/vfs.h"

typedef enum {
    PROTOCOL_TARTARUS
} protocol_t;

typedef struct {
    size_t module_count;
    char **module_paths;
    bool enable_smp;
} protocol_tartarus_config_t;

[[noreturn]] void protocol_tartarus(protocol_tartarus_config_t *config, vfs_node_t *kernel_node, fb_t *fb);
