#pragma once

#include "protocol/protocol.h"

#include <stddef.h>

typedef struct {
    const char *kernel;
    protocol_t protocol;
    union {
        protocol_tartarus_config_t protocol_tartarus;
    };
} config_boot_entry_t;

typedef struct {
    bool framebuffer_strict_rgb;
    size_t framebuffer_width;
    size_t framebuffer_height;

    size_t boot_entry_count;
    config_boot_entry_t *boot_entries;
} config_t;

extern config_t g_config;
extern size_t g_config_selected_entry;

void config_load(const char *data, size_t data_length);
