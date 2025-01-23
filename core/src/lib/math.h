#pragma once

#define MATH_DIV_CEIL(DIVIDEND, DIVISOR) (((DIVIDEND) + (DIVISOR) - 1) / (DIVISOR))
#define MATH_CEIL(VALUE, PRECISION) (MATH_DIV_CEIL((VALUE), (PRECISION)) * (PRECISION))
#define MATH_FLOOR(VALUE, PRECISION) (((VALUE) / (PRECISION)) * (PRECISION))

static inline int math_min(int a, int b) {
    return a < b ? a : b;
}

static inline int math_max(int a, int b) {
    return a > b ? a : b;
}
