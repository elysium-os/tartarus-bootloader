.section .text.init
.globl _start

# booted via opensbi
# a0 = hartid
# a1 = dtb pointer
_start:
    la sp, _stack_top

    # zero .bss
    la t0, _bss_start
    la t1, _bss_end
.loop:
    bgeu t0, t1, .exit
    sd zero, 0(t0)
    addi t0, t0, 8
    j .loop

.exit:
    call riscv64_opensbi_entry

.halt:
    wfi
    j .halt
