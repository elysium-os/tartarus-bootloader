#include "common.S"

.equ PSCI_CPU_ON, 0xC4000003

.global apinit_smc

apinit_smc:
    mov x3, x2                                  // X3=context
    mov x2, x1                                  // X2=entry
    mov x1, x0                                  // X1=mpidr
    ldr x0, =PSCI_CPU_ON                        // X0=PSCI_CPU_ON
    smc #0
    ret

.global apinit_hvc

apinit_hvc:
    mov x3, x2                                  // X3=context
    mov x2, x1                                  // X2=entry
    mov x1, x0                                  // X1=mpidr
    ldr x0, =PSCI_CPU_ON                        // X0=PSCI_CPU_ON
    hvc #0
    ret

.global g_apinit_start
.global g_apinit_end

g_apinit_start:
    msr daifset, 0b1111

    adr x5, boot_info

    ldr x0, [x5, #8]
    ldr x1, [x5, #16]
    ldr x2, [x5, #24]
    ldr x3, [x5, #32]
    ldr x4, [x5, #48]

    // Load stack pointer
    ldr x10, [x5, #40]
    msr spsel, #0
    mov sp, x10

    SWITCH_ON_EL x10, .el1, .el2, .invalid_el

.invalid_el:
    wfi
    b .invalid_el

.el1:
    CONFIGURE_TRANSLATION x10, x11, x0, x1, x2

    mov x10, #67
    stlr x10, [x5]

    b .parked

.el2:
    CONFIGURE_EL1 x10

    adr x10, .el1
    ERET_TO_EL1 x11, x10, el2

.parked:
    wfi
    b .parked

    ldar x10, [x4]
    cbnz x4, .enter
    yield
    b .parked

.enter:
    ERET_TO_EL1 x11, x10, el1

.balign 8

g_apinit_end:

boot_info:
    .init: .skip 8
    .txsz: .skip 8
    .ttbr0: .skip 8
    .ttbr1: .skip 8
    .hhdm_offset: .skip 8
    .stack: .skip 8
    .park_address: .skip 8
