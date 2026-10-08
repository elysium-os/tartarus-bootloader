#include "../common.S"

.global aarch64_protocol_tartarus_handoff

aarch64_protocol_tartarus_handoff:
    // Load stack pointer
    msr spsel, #0
    mov sp, x1

    SWITCH_ON_EL x10, .el1, .el2, .invalid_el

.invalid_el:
    wfi
    b .invalid_el

.el1:
    CONFIGURE_TRANSLATION x10, x11, x4, x2, x3

    mov x10, x0
    mov x0, x5
    mov x1, x6

    ERET_TO_EL1 x11, x10, el1

.el2:
    CONFIGURE_EL1 x10

    adr x10, .el1
    ERET_TO_EL1 x11, x10, el2
