.macro SWITCH_ON_EL scratch_reg, el1, el2, other_el
    mrs \scratch_reg, CurrentEL
    lsr \scratch_reg, \scratch_reg, #2                      // Shift right to get the current EL

    cmp \scratch_reg, #1                                    // Current EL == 1
    b.eq \el1

    cmp \scratch_reg, #2                                    // Current EL == 2
    b.eq \el2

    b \other_el
.endm

.macro CONFIGURE_EL1 scratch_reg
    // Ensure we are using Aarch64 & disable traps
    mov \scratch_reg, #(1 << 31)                            // HCR_EL2.RW=1
    msr hcr_el2, \scratch_reg

    // Disable trapping for counter-timer
    mrs \scratch_reg, cnthctl_el2
    orr \scratch_reg, \scratch_reg, #(1 << 0) | (1 << 1)    // CNTHCTL_EL2.EL1PCTEN=1, CNTHCTL_EL2.EL1PCEN=1
    msr cnthctl_el2, \scratch_reg
    msr cntvoff_el2, xzr                                    // CNTVOFF_EL2=0

    // Disable trapping for FP/SIMD
    mov \scratch_reg, #0xFF | (1 << 9) | (1 << 13)          // CPTR_EL2.RES1=1
    orr \scratch_reg, \scratch_reg, #(1 << 8)               // CPTR_EL2.TZ=1
    orr \scratch_reg, \scratch_reg, #(1 << 12)              // CPTR_EL2.TSM=1
    msr cptr_el2, \scratch_reg
    msr hstr_el2, xzr                                       // HSTR_EL2=0

    dsb sy
    isb
.endm

.macro CONFIGURE_TRANSLATION scratch_reg1, scratch_reg2, txsz, ttbr0, ttbr1
    mov \scratch_reg1, #0xFF
    msr mair_el1, \scratch_reg1

    // Configure translation
    // TCR_EL1.IRGN0=1, TCR_EL1.ORGN0=1, TCR_EL1.SH0=3, TCR_EL1.IRGN1=1, TCR_EL1.ORGN1=1, TCR_EL1.SH1=3, TCR_EL1.TG1=2
    ldr \scratch_reg2, =(0b01 << 8) | (0b01 << 10) | (0b11 << 12) | (0b01 << 24) | (0b01 << 26) | (0b11 << 28) | (0b10 << 30)

    orr \scratch_reg2, \scratch_reg2, \txsz
    lsl \txsz, \txsz, 16
    orr \scratch_reg2, \scratch_reg2, \txsz

    mrs \scratch_reg1, id_aa64mmfr0_el1
    and \scratch_reg1, \scratch_reg1, #0xF
    lsl \scratch_reg1, \scratch_reg1, #32
    orr \scratch_reg2, \scratch_reg2, \scratch_reg1         // TCR_EL1.IPS=ID_AA64MMFR0_EL1.PARange

    msr tcr_el1, \scratch_reg2

    // Load page tables
    msr ttbr0_el1, \ttbr0
    msr ttbr1_el1, \ttbr1

    // Configure system control register
    // SCTLR_EL1.M=1, SCTLR_EL1.C=1, SCTLR_EL1.SA=1, SCTLR_EL1.SA0=1, SCTLR_EL1.ITD=1, SCTLR_EL1.SED=1, SCTLR_EL1.EOS=1
    // SCTLR_EL1.I=1, SCTLR_EL1.TSCXT=1, SCTLR_EL1.EIS=1, SCTLR_EL1.SPAN=1, SCTLR_EL1.nTLSMD=1, SCTLR_EL1.LSMAOE=1
    ldr \scratch_reg1, =(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 7) | (1 << 8) | (1 << 11) | (1 << 12) | (1 << 20) | (1 << 22) | (1 << 23) | (1 << 28) | (1 << 29)
    msr sctlr_el1, \scratch_reg1

    dsb sy
    isb
.endm

.macro ERET_TO_EL1 scratch_reg, address, from
    // SPSR_EL2.M=EL1h, SPSR_EL2.F=1, SPSR_EL2.I=1, SPSR_EL2.A=1, SPSR_EL2.D=1
    mov \scratch_reg, #0b0100 | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9)
    msr spsr_\from, \scratch_reg

    msr elr_\from, \address

    eret
.endm
