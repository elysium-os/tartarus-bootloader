#include "string.h"

size_t string_length(const char *str) {
    size_t i = 0;
    while(str[i] != '\0') ++i;
    return i;
}

int string_cmp(const char *lhs, const char *rhs) {
    size_t i = 0;
    while(lhs[i] == rhs[i])
        if(lhs[i++] == '\0') return 0;
    return lhs[i] < rhs[i] ? -1 : 1;
}

bool string_eq(const char *lhs, const char *rhs) {
    return string_cmp(lhs, rhs) == 0;
}

bool string_case_eq(const char *lhs, const char *rhs) {
    size_t i = 0;
    while(((lhs[i] >= 'A' && lhs[i] <= 'Z') ? (lhs[i] + ('a' - 'A')) : lhs[i]) == ((rhs[i] >= 'A' && rhs[i] <= 'Z') ? (rhs[i] + ('a' - 'A')) : rhs[i]))
        if(lhs[i++] == '\0') return true;
    return false;
}

char *string_copy(char *dest, const char *src) {
    size_t i;
    for(i = 0; src[i] != '\0'; i++) dest[i] = src[i];
    dest[i] = src[i];
    return dest;
}

char *string_ncopy(char *dest, const char *src, size_t n) {
    size_t i;
    for(i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for(; i < n; i++) dest[i] = '\0';
    return dest;
}
