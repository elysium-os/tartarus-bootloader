#include "parse.h"

#include "config/token.h"
#include "lexer.h"
#include "memory/heap.h"
#include "tcfg.h"

static parse_status_t parse_value(lexer_t *lexer, tcfg_value_t **out);

static parse_status_t parse_field(lexer_t *lexer, tcfg_object_t *into) {
    char *key = nullptr;
    token_t key_token = lexer_advance(lexer);
    switch(key_token.kind) {
        case TOKEN_KIND_IDENTIFIER: key = lexer_extract(lexer, key_token.start, key_token.end); break;
        case TOKEN_KIND_STRING:     key = lexer_extract_string(lexer, key_token.start, key_token.end); break;
        default:                    return (parse_status_t) {.issue = "Invalid key", .start = key_token.start, .end = key_token.end};
    }

    token_t eq_token = lexer_advance(lexer);
    if(eq_token.kind != TOKEN_KIND_PUNCT_EQUAL) {
        heap_free(key);
        return (parse_status_t) {.issue = "Expected `=`", .start = eq_token.start, .end = eq_token.end};
    }

    tcfg_value_t *value = nullptr;
    parse_status_t status = parse_value(lexer, &value);
    if(status.issue != nullptr) {
        heap_free(key);
        return status;
    }

    into->field_count++;
    into->field_keys = heap_realloc(into->field_keys, into->field_count * sizeof(char *));
    into->field_values = heap_realloc(into->field_values, into->field_count * sizeof(tcfg_value_t *));

    into->field_keys[into->field_count - 1] = key;
    into->field_values[into->field_count - 1] = value;

    return (parse_status_t) {.issue = nullptr};
}

static parse_status_t parse_fields(lexer_t *lexer, tcfg_object_t **out, token_kind_t end) {
    tcfg_object_t *object = heap_alloc(sizeof(tcfg_object_t));
    object->field_count = 0;
    object->field_keys = nullptr;
    object->field_values = nullptr;

    while(true) {
        token_t token = lexer_peek(lexer);
        if(token.kind == end) {
            lexer_advance(lexer);
            break;
        }

        parse_status_t status = parse_field(lexer, object);
        if(status.issue != nullptr) {
            tcfg_free_object(object);
            return status;
        }
    }

    *out = object;

    return (parse_status_t) {.issue = nullptr};
}

static parse_status_t parse_value(lexer_t *lexer, tcfg_value_t **out) {
    tcfg_value_t *value = heap_alloc(sizeof(tcfg_value_t));

    token_t token = lexer_advance(lexer);
    switch(token.kind) {
        case TOKEN_KIND_STRING: {
            value->type = TCFG_VALUE_TYPE_STRING;
            value->string = lexer_extract_string(lexer, token.start, token.end);
        } break;

        case TOKEN_KIND_INTEGER: {
            value->type = TCFG_VALUE_TYPE_INTEGER;
            if(!lexer_extract_integer(lexer, token.start, token.end, &value->integer)) {
                heap_free(value);
                return (parse_status_t) {.issue = "Integer out of range", .start = token.start, .end = token.end};
            }
        } break;

        case TOKEN_KIND_BOOL_TRUE: {
            value->type = TCFG_VALUE_TYPE_BOOLEAN;
            value->boolean = true;
        } break;
        case TOKEN_KIND_BOOL_FALSE: {
            value->type = TCFG_VALUE_TYPE_BOOLEAN;
            value->boolean = false;
        } break;

        case TOKEN_KIND_PUNCT_BRACE_LEFT: {
            value->type = TCFG_VALUE_TYPE_OBJECT;
            parse_status_t status = parse_fields(lexer, &value->object, TOKEN_KIND_PUNCT_BRACE_RIGHT);
            if(status.issue != nullptr) {
                heap_free(value);
                return status;
            }
        } break;

        case TOKEN_KIND_PUNCT_BRACKET_LEFT: {
            value->type = TCFG_VALUE_TYPE_ARRAY;

            size_t item_count = 0;
            tcfg_value_t **items = nullptr;
            while(lexer_peek(lexer).kind != TOKEN_KIND_PUNCT_BRACKET_RIGHT) {
                tcfg_value_t *item;
                parse_status_t status = parse_value(lexer, &item);
                if(status.issue != nullptr) {
                    for(size_t i = 0; i < item_count; i++) tcfg_free_value(items[i]);
                    heap_free(items);
                    heap_free(value);
                    return status;
                }

                items = heap_realloc(items, ++item_count * sizeof(tcfg_value_t *));
                items[item_count - 1] = item;
            }
            lexer_advance(lexer);

            value->array = heap_alloc(sizeof(tcfg_array_t));
            value->array->length = item_count;
            value->array->items = items;
        } break;


        default: {
            heap_free(value);
            return (parse_status_t) {.issue = "Invalid value", .start = token.start, .end = token.end};
        } break;
    }

    *out = value;

    return (parse_status_t) {.issue = nullptr};
}

parse_status_t parse_root(lexer_t *lexer, tcfg_object_t **out) {
    return parse_fields(lexer, out, TOKEN_KIND_EOF);
}
