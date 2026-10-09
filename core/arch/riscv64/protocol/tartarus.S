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

    csrw satp, a2
    sfence.vma zero, zero

    mv a0, a3
    mv a1, a4

    # preserve sp, t0 (entry), a0 (boot_info), a1 (version).
    li ra, 0
    li gp, 0
    li tp, 0

    li t1, 0
    li t2, 0
    li t3, 0
    li t4, 0
    li t5, 0
    li t6, 0

    li s0, 0
    li s1, 0
    li s2, 0
    li s3, 0
    li s4, 0
    li s5, 0
    li s6, 0
    li s7, 0
    li s8, 0
    li s9, 0
    li s10, 0
    li s11, 0

    jr      t0
