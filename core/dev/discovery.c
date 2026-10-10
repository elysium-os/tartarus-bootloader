#include "discovery.h"

#include "common/panic.h"
#include "dev/disk.h"

#if defined(__PLATFORM_X86_64_BIOS)
#include "arch/x86_64/bios/disk.h"
#elif defined(__UEFI)
#include "arch/uefi/disk.h"
#else
#include "dev/dtb.h"
#include "dev/virtio/virtio_blk.h"
#endif

void device_discover() {
#if defined(__PLATFORM_X86_64_BIOS)
    bios_disk_initialize();
#elif defined(__UEFI)
    uefi_disk_initialize();
#else
    dtb_init_devices();
    virtio_blk_initialize();
#endif

    if(g_disks == nullptr) panic("Failed to find usable block devices");
}
