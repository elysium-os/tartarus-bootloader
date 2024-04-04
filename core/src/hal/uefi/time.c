#include <hal/time.h>
#include <hal/uefi/efi.h>
#include <common/log.h>

time_t hal_time() {
    EFI_TIME time;
    if(EFI_ERROR(g_system_table->RuntimeServices->GetTime(&time, NULL))) return 0;
    return time_datetime_to_unix_time(time.Year, time.Month, time.Day, time.Hour, time.Minute, time.Second);
}