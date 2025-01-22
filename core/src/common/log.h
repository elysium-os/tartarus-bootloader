#pragma once

#include <stdarg.h>

typedef enum {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
} log_level_t;

typedef void (*log_sink_t)(char c);

void log_sink_set(log_sink_t sink);

void log(log_level_t level, const char *fmt, ...);
void log_list(log_level_t level, const char *fmt, va_list list);