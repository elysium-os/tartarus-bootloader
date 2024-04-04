#pragma once
#include <stdint.h>

typedef uint64_t time_t;

time_t time_datetime_to_unix_time(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second);