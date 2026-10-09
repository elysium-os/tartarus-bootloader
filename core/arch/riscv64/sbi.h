#pragma once
#include <stdint.h>

typedef struct {
    long error;
    long value;
} sbiret_t;

#define SBI_EXT_LEGACY_PUTCHAR 1
#define SBI_EXT_HSM 0x48534D

void sbi_legacy_putc(char ch);
sbiret_t sbi_start_hart(uint64_t hart_id, uint64_t start_address, uint64_t param);
