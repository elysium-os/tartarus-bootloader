#pragma once

#include "arch/fb.h"
#include "common/config.h"
#include "fs/vfs.h"

[[noreturn]] void protocol_linux(config_t *config, vfs_node_t *kernel_node, fb_t *fb);
[[noreturn]] void protocol_tartarus(config_t *config, vfs_node_t *kernel_node, fb_t *fb);