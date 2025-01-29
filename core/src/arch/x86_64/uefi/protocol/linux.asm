bits 64

section .data
align 16
linux_gdt:
    dq 0
    dq 0

    dw 0x0000
    dw 0x0000
    db 0x00
    db 10011011b
    db 00100000b
    db 0x00

    dw 0x0000
    dw 0x0000
    db 0x00
    db 10010011b
    db 00000000b
    db 0x00
.gdtr:
    dw (linux_gdt.gdtr - linux_gdt) - 1
    dq linux_gdt

section .text
global linux_handoff
linux_handoff:
    cli

    lgdt [rel linux_gdt.gdtr]

    lea rax, [rel .loadcs]
    push 0x10
    push rax
    retfq

.loadcs:
    mov rax, 0x18
    mov ds, rax
    mov es, rax
    mov ss, rax
    cld

    add rdi, 0x200
    jmp rdi