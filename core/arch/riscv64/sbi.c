#include "sbi.h"

#include <stdint.h>

inline sbiret_t sbi_call(long eid, long fid, long a0, long a1, long a2) {
    register long r0 asm("a0") = a0;
    register long r1 asm("a1") = a1;
    register long r2 asm("a2") = a2;
    register long r6 asm("a6") = fid;
    register long r7 asm("a7") = eid;
    asm volatile("ecall" : "+r"(r0), "+r"(r1) : "r"(r2), "r"(r6), "r"(r7) : "memory");
    return (sbiret_t) {r0, r1};
}

#define SBI_EXT_LEGACY_PUTCHAR 1
#define SBI_EXT_HSM 0x48534D

void sbi_legacy_putc(char ch) {
    sbi_call(SBI_EXT_LEGACY_PUTCHAR, 0, ch, 0, 0);
}

sbiret_t sbi_start_hart(uint64_t hart_id, uint64_t start_address, uint64_t param) {
    return sbi_call(SBI_EXT_HSM, 0, hart_id, start_address, param);
}
