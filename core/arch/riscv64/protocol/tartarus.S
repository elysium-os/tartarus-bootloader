.section .text
.globl riscv64_protocol_tartarus_handoff
riscv64_protocol_tartarus_handoff:
    # Arguments on entry:
    # a0 = entry
    # a1 = stack
    # a2 = satp
    # a3 = boot_info
    # a4 = version (uint16_t)

    mv t0, a0
    mv sp, a1

    li t1, 0x200000000
    csrw sstatus, t1
    csrw stvec, 0
    csrw sie, 0

    csrw satp, a2
    sfence.vma zero, zero

    mv a0, a3
    mv a1, a4

    # preserve sp, t0 (entry), a0 (boot_info), a1 (version).
    mv x1, x0 # ra
    # x2 (sp)
    mv x3, x0 # gp
    mv x4, x0 # tp
    # x5 (t0)
    mv x6, x0
    mv x7, x0
    mv x8, x0
    mv x9, x0
    # x10 (a0)
    # x11 (a1)
    mv x12, x0
    mv x13, x0
    mv x14, x0
    mv x15, x0
    mv x16, x0
    mv x17, x0
    mv x18, x0
    mv x19, x0
    mv x20, x0
    mv x21, x0
    mv x22, x0
    mv x23, x0
    mv x24, x0
    mv x25, x0
    mv x26, x0
    mv x27, x0
    mv x28, x0
    mv x29, x0
    mv x30, x0
    mv x31, x0

    jr      t0
