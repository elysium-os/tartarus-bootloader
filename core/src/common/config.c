#include "config.h"

#include "common/log.h"
#include "common/panic.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"

#define IS_WHITESPACE(CHAR) ((CHAR) == ' ' || (CHAR) == '\t')
#define IS_ALPHA(CHAR) (((CHAR) >= 'a' && (CHAR) <= 'z') || ((CHAR) >= 'A' && (CHAR) <= 'Z'))
#define IS_NUMERIC(CHAR) ((CHAR) >= '0' && (CHAR) <= '9')

typedef enum {
    TOKEN_KIND_EOF,
    TOKEN_KIND_STRING,
    TOKEN_KIND_NUMBER,
    TOKEN_KIND_IDENTIFIER,
    TOKEN_KIND_EQUAL,
    TOKEN_KIND_COMMA,
    TOKEN_KIND_NEWLINE
} token_kind_t;

typedef struct {
    token_kind_t kind;
    size_t start, length;
} token_t;

typedef struct {
    const char *data;
    size_t size, cursor;
} buffer_t;

static config_entry_t *find_entry(config_t *config, config_entry_type_t type, const char *key, size_t index) {
    for(size_t i = 0; i < config->entry_count; i++) {
        if(config->entries[i].type != type || !string_case_eq(config->entries[i].key, key)) continue;
        if(index != 0) {
            index--;
            continue;
        }
        return &config->entries[i];
    }
    return NULL;
}

static token_t read_token(buffer_t *buffer) {
    while(buffer->cursor != buffer->size && IS_WHITESPACE(buffer->data[buffer->cursor])) buffer->cursor++;

    token_t token = {.start = buffer->cursor};
    if(buffer->cursor == buffer->size) panic("config parse failed: EOF");

    if(IS_NUMERIC(buffer->data[buffer->cursor])) {
        while(buffer->cursor != buffer->size && IS_NUMERIC(buffer->data[buffer->cursor])) buffer->cursor++;
        token.length = buffer->cursor - token.start;
        token.kind = TOKEN_KIND_NUMBER;
        return token;
    }

    if(buffer->data[buffer->cursor] == '_' || IS_ALPHA(buffer->data[buffer->cursor])) {
        while(buffer->cursor != buffer->size && (buffer->data[buffer->cursor] == '_' || IS_ALPHA(buffer->data[buffer->cursor]) || IS_NUMERIC(buffer->data[buffer->cursor]))) buffer->cursor++;
        token.length = buffer->cursor - token.start;
        token.kind = TOKEN_KIND_IDENTIFIER;
        return token;
    }

    if(buffer->data[buffer->cursor] == '"') {
        buffer->cursor++;
        while(buffer->cursor != buffer->size && buffer->data[buffer->cursor] != '"' && buffer->data[buffer->cursor] != '\n') buffer->cursor++;
        if(buffer->cursor == buffer->size || buffer->data[buffer->cursor] != '"') panic("config parse failed: incomplete string");
        buffer->cursor++;

        token.start++;
        token.length = buffer->cursor - token.start - 1;
        token.kind = TOKEN_KIND_STRING;
        return token;
    }

    switch(buffer->data[buffer->cursor]) {
        case '\n': token.kind = TOKEN_KIND_NEWLINE; break;
        case '=':  token.kind = TOKEN_KIND_EQUAL; break;
        case ',':  token.kind = TOKEN_KIND_COMMA; break;
        default:   panic("config parse failed: unexpected symbol `%c`", buffer->data[buffer->cursor]);
    }
    buffer->cursor++;
    token.length = 1;
    return token;
}

config_t *config_parse(vfs_node_t *config_node) {
    config_t *config = heap_alloc(sizeof(config_t));
    config->entry_count = 0;
    config->entries = NULL;

    size_t config_size = config_node->ops->get_size(config_node);
    char *buffer_data = heap_alloc(config_size);
    config_node->ops->read(config_node, buffer_data, 0, config_size);

    buffer_t buffer = {.data = buffer_data, .size = config_size, .cursor = 0};
    bool expect_newline = false;
    while(buffer.cursor != buffer.size) {
        token_t token = read_token(&buffer);
        if(expect_newline && token.kind != TOKEN_KIND_NEWLINE) panic("config parse failed: expected newline got `%.*s`", token.length, &buffer.data[token.start]);
        expect_newline = false;

        if(token.kind == TOKEN_KIND_NEWLINE) continue;
        if(token.kind == TOKEN_KIND_IDENTIFIER) {
            if(read_token(&buffer).kind != TOKEN_KIND_EQUAL) panic("config parse failed: expected `=` got `%.*s`", token.length, &buffer.data[token.start]);
            token_t token_value = read_token(&buffer);

            char *key = heap_alloc(token.length + 1);
            memcpy(key, &buffer.data[token.start], token.length);
            key[token.length] = '\0';

            config->entries = heap_realloc(config->entries, sizeof(config_entry_t) * ++config->entry_count);
            config->entries[config->entry_count - 1].key = key;

            switch(token_value.kind) {
                case TOKEN_KIND_IDENTIFIER: {
                    bool value;

                    char *strval = heap_alloc(token_value.length + 1);
                    memcpy(strval, &buffer.data[token_value.start], token_value.length);
                    strval[token_value.length] = '\0';
                    if(string_case_eq(strval, "true")) {
                        value = true;
                    } else if(string_case_eq(strval, "false")) {
                        value = false;
                    } else {
                        panic("config parse failed: invalid config entry token `%.*s`", token_value.length, &buffer.data[token_value.start]);
                    }
                    heap_free(strval);

                    config->entries[config->entry_count - 1].type = CONFIG_ENTRY_TYPE_BOOLEAN;
                    config->entries[config->entry_count - 1].value.boolean = value;
                } break;
                case TOKEN_KIND_STRING: {
                    char *value = heap_alloc(token_value.length + 1);
                    memcpy(value, &buffer.data[token_value.start], token_value.length);
                    value[token_value.length] = '\0';
                    config->entries[config->entry_count - 1].type = CONFIG_ENTRY_TYPE_STRING;
                    config->entries[config->entry_count - 1].value.string = value;
                } break;
                case TOKEN_KIND_NUMBER: {
                    uintmax_t value = 0;
                    for(size_t i = 0; i < token_value.length; i++) {
                        value *= 10;
                        value += buffer.data[token_value.start + i] - '0';
                    }
                    config->entries[config->entry_count - 1].type = CONFIG_ENTRY_TYPE_NUMBER;
                    config->entries[config->entry_count - 1].value.number = value;
                } break;
                default: panic("config parse failed: invalid config entry token `%.*s`", token_value.length, &buffer.data[token_value.start]);
            }
            expect_newline = true;
        }
    }

    heap_free(buffer_data);
    return config;
}

size_t config_key_count(config_t *config, const char *key, config_entry_type_t type) {
    size_t count = 0;
    while(find_entry(config, type, key, count) != NULL) count++;
    return count;
}

const char *config_find_string(config_t *config, const char *key, const char *default_value) {
    return config_find_string_at(config, key, default_value, 0);
}

const char *config_find_string_at(config_t *config, const char *key, const char *default_value, size_t index) {
    config_entry_t *entry = find_entry(config, CONFIG_ENTRY_TYPE_STRING, key, index);
    if(entry == NULL) return default_value;
    return entry->value.string;
}

uintmax_t config_find_number(config_t *config, const char *key, uintmax_t default_value) {
    return config_find_number_at(config, key, default_value, 0);
}

uintmax_t config_find_number_at(config_t *config, const char *key, uintmax_t default_value, size_t index) {
    config_entry_t *entry = find_entry(config, CONFIG_ENTRY_TYPE_NUMBER, key, index);
    if(entry == NULL) return default_value;
    return entry->value.number;
}

bool config_find_bool(config_t *config, const char *key, bool default_value) {
    return config_find_bool_at(config, key, default_value, 0);
}

bool config_find_bool_at(config_t *config, const char *key, bool default_value, size_t index) {
    config_entry_t *entry = find_entry(config, CONFIG_ENTRY_TYPE_BOOLEAN, key, index);
    if(entry == NULL) return default_value;
    return entry->value.boolean;
}
