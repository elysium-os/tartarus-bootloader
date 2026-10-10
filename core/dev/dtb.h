#pragma once
#include <stdint.h>

bool dtb_early_init(void *dtb_pointer);
void dtb_init_devices();
void dtb_map_device_tree();
