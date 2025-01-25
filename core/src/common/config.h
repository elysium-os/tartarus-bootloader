#pragma once

#include "fs/vfs.h"

#include <stdint.h>

typedef enum {
    CONFIG_ENTRY_TYPE_STRING,
    CONFIG_ENTRY_TYPE_NUMBER
} config_entry_type_t;

typedef struct {
    config_entry_type_t type;
    const char *key;
    union {
        const char *string;
        uintmax_t number;
    } value;
} config_entry_t;

typedef struct {
    config_entry_t *entries;
    size_t entry_count;
} config_t;

config_t *config_parse(vfs_node_t *config_node);
bool config_find_string(config_t *config, const char *key, const char **out);
bool config_find_number(config_t *config, const char *key, uintmax_t *out);
