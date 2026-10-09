.global g_apinit_start

g_apinit_start:
    ld sp, 8(a1)

    li t0, 0x200000000
    csrw sstatus, t0

    ld t0, 16(a1)
    csrw satp, t0
    sfence.vma zero, zero

    csrw stvec, 0

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

    li ra, 0
    li gp, 0
    li tp, 0

    li t0, 0
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

    jr t1
