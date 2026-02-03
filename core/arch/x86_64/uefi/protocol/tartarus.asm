global x86_64_protocol_tartarus_handoff

bits 64
x86_64_protocol_tartarus_handoff:
    mov cr3, rdx                                ; Load page tables

    mov rax, cr0
    or rax, (1 << 16)                           ; Set write protect bit
    mov cr0, rax

    xor rbp, rbp
    mov rsp, rsi                                ; Load stack from rsi
    push qword 0                                ; Push an invalid return address

    mov rax, rdi                                ; Move entry_address into rax
    mov rdi, rcx                                ; Move boot_info into rdi
    mov rsi, r8                                 ; Move version into rsi

    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
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
