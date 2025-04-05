#include "arch/time.h"

#include "lib/time.h"

#include "arch/x86_64/uefi/uefi.h"

time_t arch_time() {
    EFI_TIME time;
    EFI_TIME_CAPABILITIES capabilities;
    EFI_STATUS status = g_x86_64_uefi_efi_system_table->RuntimeServices->GetTime(&time, &capabilities);
    if(EFI_ERROR(status)) return 0;
    return time_datetime_to_unix_time(time.Year, time.Month, time.Day, time.Hour, time.Minute, time.Second);
}
