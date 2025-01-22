#pragma once

#include <stdarg.h>

typedef void (*format_writer_t)(char ch);

int format(format_writer_t writer, const char *format, va_list list);
