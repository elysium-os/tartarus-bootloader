#pragma once

#include <stdarg.h>

typedef enum {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
} log_level_t;

typedef struct log_sink {
    log_level_t level;
    void (*char_out)(char ch);
    struct log_sink *next;
} log_sink_t;

void log_sink_add(log_sink_t *sink);

void log(log_level_t level, const char *fmt, ...);
void log_list(log_level_t level, const char *fmt, va_list list);