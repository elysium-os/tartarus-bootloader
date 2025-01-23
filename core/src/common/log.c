#include "log.h"

#include "lib/format.h"

#include <stddef.h>
#include <stdint.h>

static log_sink_t g_sink = NULL;

static void internal_fmt_list(const char *fmt, va_list list) {
    if(g_sink != NULL) format(g_sink, fmt, list);
}

static void internal_fmt(const char *fmt, ...) {
    va_list list;
    va_start(list, str);
    internal_fmt_list(fmt, list);
    va_end(list);
}

static const char *log_level_stringify(log_level_t level) {
    switch(level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
    }
    return "UNKNOWN";
}

void log_sink_set(log_sink_t sink) {
    g_sink = sink;
}

void log_list(log_level_t level, const char *fmt, va_list list) {
    internal_fmt("[TARTARUS::%s] ", log_level_stringify(level));
    internal_fmt_list(fmt, list);
    internal_fmt("\n");
}

void log(log_level_t level, const char *fmt, ...) {
    va_list list;
    va_start(list, str);
    log_list(level, fmt, list);
    va_end(list);
}
