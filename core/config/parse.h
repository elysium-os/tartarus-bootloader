#pragma once

#include "lexer.h"
#include "tcfg.h"

#include <stddef.h>

typedef struct {
    const char *issue;
    size_t start, end;
} parse_status_t;

parse_status_t parse_root(lexer_t *lexer, tcfg_object_t **out);
