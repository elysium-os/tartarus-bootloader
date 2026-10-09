.global g_apinit_start

g_apinit_start:
    ld sp, 8(a1)

    li t1, 0x200000000
    csrw sstatus, t1
    csrw stvec, 0
    csrw sie, 0

    ld t0, 16(a1)
    csrw satp, t0
    sfence.vma zero, zero

    li t1, 67
    amoswap.d.rl zero, t1, (a1)

.loop:
    ld t0, 24(a1)           # t0 = park_address
    ld t1, 0(t0)            # t1 = entry point
    beqz t1, .wait
    fence r, rw
    j .handoff

.wait:
    fence r, rw
    pause
    j .loop

.handoff:
    ld a0, 8(t0)            # a0 = argument
    mv t0, t1

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
    mv x11, x0
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

    jr t0
