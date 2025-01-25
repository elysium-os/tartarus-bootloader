#pragma once

#include "fs/vfs.h"

#include <stdint.h>

typedef enum {
    CONFIG_ENTRY_TYPE_STRING,
    CONFIG_ENTRY_TYPE_NUMBER,
    CONFIG_ENTRY_TYPE_BOOLEAN
} config_entry_type_t;

typedef struct {
    config_entry_type_t type;
    const char *key;
    union {
        const char *string;
        uintmax_t number;
        bool boolean;
    } value;
} config_entry_t;

typedef struct {
    config_entry_t *entries;
    size_t entry_count;
} config_t;

config_t *config_parse(vfs_node_t *config_node);

const char *config_find_string(config_t *config, const char *key, const char *default_value);
uintmax_t config_find_number(config_t *config, const char *key, uintmax_t default_value);
bool config_find_bool(config_t *config, const char *key, bool default_value);
