#pragma once

#include <stddef.h>

size_t string_length(const char *str);
int string_cmp(const char *lhs, const char *rhs);
bool string_eq(const char *lhs, const char *rhs);
bool string_case_eq(const char *lhs, const char *rhs);
char *string_copy(char *dest, const char *src);
char *string_ncopy(char *dest, const char *src, size_t n);
