extern g_x86_64_gdt
extern g_x86_64_gdt_limit
extern g_x86_64_cpu_la57_support

global x86_64_protocol_tartarus_handoff

bits 64
x86_64_protocol_tartarus_handoff:
    movzx rbx, byte [rel g_x86_64_cpu_la57_support]
    test bl, bl
    jz nola57

    lea rax, [rel g_x86_64_gdt]
    push rax
    mov ax, [rel g_x86_64_gdt_limit]
    push ax
    lgdt [rsp]
    add rsp, 10

    lea rax, [rel .entry_protected]
    lea rbx, [rel entry_long]
    push qword 0x18
    push rax
    retfq

bits 32
.entry_protected:
    mov eax, cr0
    btr eax, 31            ; clear PG
    mov cr0, eax

    mov ax, 0x20
    mov ds, ax
    mov ss, ax

    mov eax, cr4
    or eax, (1 << 12)                           ; Enable 5 level paging
    mov cr4, eax

    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax                                ; Enable paging

    mov cr3, edx                                ; Load page tables

    push dword 0x28
    push ebx
    retf
bits 64
entry_long:
    mov rax, 0x30                               ; Reset segments
    mov ss, rax

    xor rax, rax
    mov ds, rax
    mov es, rax
    mov fs, rax
    mov gs, rax
nola57:
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

    push qword 0x2
    popfq

    jmp rax
