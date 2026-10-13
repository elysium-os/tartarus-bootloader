#pragma once

#include <stdarg.h>

#define LOGFMT_U64 __UINT64_FMTu__
#define LOGFMT_UPTR __UINTPTR_FMTu__
#define LOGFMT_X64 __UINT64_FMTx__
#define LOGFMT_XPTR __UINTPTR_FMTx__

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
void log_sink_remove(log_sink_t *sink);

[[gnu::format(printf, 2, 3)]] void log(log_level_t level, const char *fmt, ...);
void log_list(log_level_t level, const char *fmt, va_list list);
