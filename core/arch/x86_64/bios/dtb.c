#include "lib/mem.h"

void *arch_dtb_find_dtb() {
    // afaik, there is no way to get a device tree on bios devices
    return nullptr;
}
