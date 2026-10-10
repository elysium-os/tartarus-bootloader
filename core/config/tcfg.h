#pragma once

#include <stddef.h>

typedef enum {
    TCFG_VALUE_TYPE_OBJECT,
    TCFG_VALUE_TYPE_ARRAY,
    TCFG_VALUE_TYPE_STRING,
    TCFG_VALUE_TYPE_INTEGER,
    TCFG_VALUE_TYPE_BOOLEAN,
} tcfg_value_type_t;

typedef struct tcfg_value tcfg_value_t;
typedef struct tcfg_array tcfg_array_t;
typedef struct tcfg_object tcfg_object_t;

struct tcfg_value {
    tcfg_value_type_t type;
    union {
        tcfg_object_t *object;
        tcfg_array_t *array;
        char *string;
        int integer;
        bool boolean;
    };
};

struct tcfg_array {
    size_t length;
    tcfg_value_t **items;
};

struct tcfg_object {
    size_t field_count;
    char **field_keys;
    tcfg_value_t **field_values;
};

void tcfg_free_object(tcfg_object_t *object);
void tcfg_free_array(tcfg_array_t *array);
void tcfg_free_value(tcfg_value_t *value);

tcfg_value_t *tcfg_object_get_field(tcfg_object_t *object, const char *field_name);
