#pragma once
#include <stdint.h>

typedef struct {
    // dtb and rsdp can be null
    void *dtb;
    void *rsdp;

    // lapic, mpidr, hartid of the boot core
    uint64_t boot_cpu_id;
} firmware_t;

firmware_t *firmware_get();

void *firmware_get_rdsp();
void *firmware_get_dtb();
