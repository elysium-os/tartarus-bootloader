#pragma once

#include <stddef.h>

typedef enum {
    TOKEN_KIND_INVALID,
    TOKEN_KIND_EOF,
    TOKEN_KIND_IDENTIFIER,
    TOKEN_KIND_STRING,
    TOKEN_KIND_INTEGER,
    TOKEN_KIND_BOOL_TRUE,
    TOKEN_KIND_BOOL_FALSE,
    TOKEN_KIND_PUNCT_EQUAL,
    TOKEN_KIND_PUNCT_BRACE_LEFT,
    TOKEN_KIND_PUNCT_BRACE_RIGHT,
    TOKEN_KIND_PUNCT_BRACKET_LEFT,
    TOKEN_KIND_PUNCT_BRACKET_RIGHT,
} token_kind_t;

typedef struct {
    token_kind_t kind;
    size_t start, end;
} token_t;
