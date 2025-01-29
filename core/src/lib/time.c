#include "time.h"

static int date_to_julian_day_number(int year, int month, int day) {
    // Got this calculation from https://en.wikipedia.org/wiki/Julian_day#Converting_Gregorian_calendar_date_to_Julian_Day_Number
    return (1461 * (year + 4800 + (month - 14) / 12)) / 4 + (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12 - (3 * ((year + 4900 + (month - 14) / 12) / 100)) / 4 + day - 32075;
}

time_t time_datetime_to_unix_time(unsigned int year, unsigned int month, unsigned int day, unsigned int hour, unsigned int minute, unsigned int second) {
    uint64_t current_jdn = date_to_julian_day_number(year, month, day);
    uint64_t unix_jdn = date_to_julian_day_number(1970, 1, 1);
    return ((current_jdn - unix_jdn) * 86400) + (hour * 3600) + (minute * 60) + second;
}
