#pragma once

#include "token.h"

#include <stddef.h>

typedef struct {
    const char *data;
    size_t data_length;
    size_t cursor;
    token_t lookahead;
} lexer_t;

lexer_t lexer_new(const char *data, size_t data_length);
token_t lexer_peek(lexer_t *lexer);
token_t lexer_advance(lexer_t *lexer);

char *lexer_extract(lexer_t *lexer, size_t start, size_t end);
char *lexer_extract_string(lexer_t *lexer, size_t start, size_t end);
bool lexer_extract_integer(lexer_t *lexer, size_t start, size_t end, int *out);
