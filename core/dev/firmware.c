#include "firmware.h"

#include "arch/acpi.h"
#include "arch/dtb.h"
#include "dev/dtb.h"

static firmware_t g_firmware;

firmware_t *firmware_get() {
    return &g_firmware;
}


void *firmware_get_rdsp() {
    static bool cached = false;
    if(cached || g_firmware.rsdp != nullptr) { return g_firmware.rsdp; }

    g_firmware.rsdp = arch_acpi_find_rsdp();
    cached = true;
    return g_firmware.rsdp;
}

void *firmware_get_dtb() {
    static bool cached = false;
    if(cached || g_firmware.dtb != nullptr) { return g_firmware.dtb; }

    g_firmware.dtb = arch_dtb_find_dtb();
    cached = true;

    if(g_firmware.dtb != nullptr) { dtb_early_init(g_firmware.dtb); }

    return g_firmware.dtb;
}
