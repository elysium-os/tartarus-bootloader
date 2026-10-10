#include "lexer.h"

#include "common/assert.h"
#include "config/token.h"
#include "lib/mem.h"
#include "lib/string.h"
#include "memory/heap.h"

#include <limits.h>

static token_t next_token(const char *data, size_t data_length, size_t *cursor) {
    enum state {
        STATE_INITIAL,
        STATE_IDENT,
        STATE_INT_SIGN,
        STATE_INT,
        STATE_STR,
        STATE_STR_ESCAPE,
    };

    token_t token = {
        .kind = TOKEN_KIND_INVALID,
        .start = *cursor,
        .end = *cursor,
    };

    if(*cursor >= data_length) {
        token.kind = TOKEN_KIND_EOF;
        return token;
    }

    enum state state = STATE_INITIAL;
    while(*cursor < data_length) {
        char ch = data[*cursor];

        switch(state) {
            case STATE_INITIAL: {
                switch(ch) {
                    case 'a' ... 'z':
                    case 'A' ... 'Z':
                    case '_':         state = STATE_IDENT; break;

                    case '+':         state = STATE_INT_SIGN; break;
                    case '-':         state = STATE_INT_SIGN; break;
                    case '0' ... '9': state = STATE_INT; break;

                    case '"': state = STATE_STR; break;

                    case '=': token.kind = TOKEN_KIND_PUNCT_EQUAL; goto terminate_consume;
                    case '{': token.kind = TOKEN_KIND_PUNCT_BRACE_LEFT; goto terminate_consume;
                    case '}': token.kind = TOKEN_KIND_PUNCT_BRACE_RIGHT; goto terminate_consume;
                    case '[': token.kind = TOKEN_KIND_PUNCT_BRACKET_LEFT; goto terminate_consume;
                    case ']': token.kind = TOKEN_KIND_PUNCT_BRACKET_RIGHT; goto terminate_consume;

                    case ' ':
                    case '\t':
                    case '\n':
                    case '\r':
                        (*cursor)++;
                        token.start = *cursor;

                        if(*cursor >= data_length) {
                            token.kind = TOKEN_KIND_EOF;
                            goto terminate;
                        }
                        continue;

                    default: goto terminate;
                };
            }; break;

            case STATE_IDENT: {
                switch(ch) {
                    case 'a' ... 'z':
                    case 'A' ... 'Z':
                    case '0' ... '9':
                    case '_':         break;
                    default:          token.kind = TOKEN_KIND_IDENTIFIER; goto terminate;
                };
            }; break;

            case STATE_INT_SIGN: {
                switch(ch) {
                    case '0' ... '9': token.kind = TOKEN_KIND_INTEGER; break;
                    default:          goto terminate;
                };
            } break;

            case STATE_INT: {
                switch(ch) {
                    case '0' ... '9':
                    case '_':         break;
                    default:          token.kind = TOKEN_KIND_INTEGER; goto terminate;
                };
            }; break;

            case STATE_STR: {
                switch(ch) {
                    case '\\': state = STATE_STR_ESCAPE; break;
                    case '"':  token.kind = TOKEN_KIND_STRING; goto terminate_consume;
                    default:   break;
                };
            }; break;

            case STATE_STR_ESCAPE: state = STATE_STR; break;
        }

        (*cursor)++;
    }

terminate_consume:
    (*cursor)++;
terminate:
    token.end = *cursor;

    if(token.kind == TOKEN_KIND_IDENTIFIER) {
        if(string_ncmp(&data[token.start], "true", token.end - token.start) == 0) {
            token.kind = TOKEN_KIND_BOOL_TRUE;
        } else if(string_ncmp(&data[token.start], "false", token.end - token.start) == 0) {
            token.kind = TOKEN_KIND_BOOL_FALSE;
        }
    }

    return token;
}

char *lexer_extract(lexer_t *lexer, size_t start, size_t end) {
    ASSERT(start < lexer->data_length);
    ASSERT(end <= lexer->data_length);
    ASSERT(end > start);

    size_t len = end - start;
    char *str = heap_alloc(len + 1);
    memcpy(str, &lexer->data[start], len);
    str[len] = '\0';

    return str;
}

char *lexer_extract_string(lexer_t *lexer, size_t start, size_t end) {
    ASSERT(start < lexer->data_length);
    ASSERT(end <= lexer->data_length);
    ASSERT(end > start && end - start >= 2);

    size_t strlen = 0;
    char *str = nullptr;

    bool bkslsh = false;
    for(size_t i = start + 1; i < end; i++) {
        char ch = lexer->data[i];
        if(ch == '\\' && !bkslsh) {
            bkslsh = true;
            continue;
        }

        str = heap_realloc(str, ++strlen);
        str[strlen - 1] = ch;

        bkslsh = false;
    }

    str = heap_realloc(str, strlen + 1);
    str[strlen - 1] = '\0';

    return str;
}

bool lexer_extract_integer(lexer_t *lexer, size_t start, size_t end, int *out) {
    ASSERT(start < lexer->data_length);
    ASSERT(end <= lexer->data_length);
    ASSERT(end > start);

    size_t i = start;
    bool negative = false;

    if(lexer->data[i] == '-' || lexer->data[i] == '+') {
        negative = (lexer->data[i] == '-');
        i++;
    }

    int value = 0;
    for(; i < end; i++) {
        int digit = lexer->data[i] - '0';
        if(value < (INT_MIN + digit) / 10) return false;
        value = value * 10 - digit;
    }

    if(!negative) {
        if(value < -INT_MAX) return false;
        value = -value;
    }

    *out = value;

    return true;
}

lexer_t lexer_new(const char *data, size_t data_length) {
    lexer_t lexer = {
        .data = data,
        .data_length = data_length,
        .cursor = 0,
    };
    lexer.lookahead = next_token(lexer.data, lexer.data_length, &lexer.cursor);
    return lexer;
}

token_t lexer_peek(lexer_t *lexer) {
    return lexer->lookahead;
}

token_t lexer_advance(lexer_t *lexer) {
    token_t prev = lexer->lookahead;
    lexer->lookahead = next_token(lexer->data, lexer->data_length, &lexer->cursor);
    return prev;
}
