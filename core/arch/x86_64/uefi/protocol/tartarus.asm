global protocol_tartarus_handoff

bits 64
protocol_tartarus_handoff:
    mov rax, cr0
    or rax, (1 << 16)                           ; Set write protect bit
    mov cr0, rax

    mov rax, qword rdi                          ; Move entry_address into rax
    mov rdi, rdx                                ; Move boot_info into rdi

    xor rbp, rbp
    mov rsp, qword rsi                          ; Move stack into rsi
    push qword 0                                ; Push an invalid return address

    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    cld

    jmp rax