#include "log.h"

#include "lib/format.h"

#include <stddef.h>
#include <stdint.h>

static log_sink_t *g_sinks = NULL;

static void internal_fmt_list(log_level_t level, const char *fmt, va_list list) {
    va_list local_list;
    for(log_sink_t *sink = g_sinks; sink != NULL; sink = sink->next) {
        if(sink->level > level) continue;
        va_copy(local_list, list);
        format(sink->char_out, fmt, local_list);
        va_end(local_list);
    }
}

static void internal_fmt(log_level_t level, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    internal_fmt_list(level, fmt, list);
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

void log_sink_add(log_sink_t *sink) {
    sink->next = g_sinks;
    g_sinks = sink;
}

void log_list(log_level_t level, const char *fmt, va_list list) {
    internal_fmt(level, "[TARTARUS::%s] ", log_level_stringify(level));
    internal_fmt_list(level, fmt, list);
    internal_fmt(level, "\n");
}

void log(log_level_t level, const char *fmt, ...) {
    va_list list;
    va_start(list, str);
    log_list(level, fmt, list);
    va_end(list);
}
