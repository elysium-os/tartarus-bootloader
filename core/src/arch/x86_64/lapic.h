#pragma once

#include <stdint.h>

uint32_t x86_64_lapic_id();
void x86_64_lapic_ipi_init(uint8_t lapic_id);
void x86_64_lapic_ipi_startup(uint8_t lapic_id, void *startup_page);