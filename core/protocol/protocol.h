#pragma once

#include "arch/fb.h"
#include "common/config.h"
#include "fs/vfs.h"

typedef struct {
    const char *name;
    void (*entry)(config_t *config, vfs_node_t *kernel_node, fb_t *fb);
} protocol_t;

protocol_t *protocol_match(const char *name);
