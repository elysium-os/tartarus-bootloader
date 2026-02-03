global x86_64_protocol_tartarus_handoff

bits 32
x86_64_protocol_tartarus_handoff:
    mov eax, dword [esp + 4]
    mov dword [kernel_entry], eax
    mov eax, dword [esp + 8]
    mov dword [kernel_entry + 4], eax

    mov eax, dword [esp + 12]
    mov dword [stack], eax
    mov eax, dword [esp + 16]
    mov dword [stack + 4], eax

    mov eax, dword [esp + 20]
    mov dword [top_page_table], eax

    mov eax, dword [esp + 28]
    mov dword [boot_info], eax
    mov eax, dword [esp + 32]
    mov dword [boot_info + 4], eax

    mov eax, dword [esp + 36]
    mov dword [version], eax
    mov eax, dword [esp + 40]
    mov dword [version + 4], eax

    mov eax, cr4
    or eax, 1 << 5                              ; Enable PAE bit
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8                              ; Set long mode bit
    wrmsr                                       ; Enable long mode

    mov eax, dword [top_page_table]
    mov cr3, eax                                ; Load page tables

    mov eax, cr0
    or eax, (1 << 31) | (1 << 16)               ; Set paging bit & write protect bits
    mov cr0, eax                                ; Enable paging

    jmp 0x28:entry_long                         ; Long jump into long mode gdt segment

bits 64
entry_long:
    mov rax, 0x30                               ; Reset segments
    mov ss, rax

    xor rax, rax
    mov ds, rax
    mov es, rax
    mov fs, rax
    mov gs, rax

    mov rax, qword [kernel_entry]
    mov rdi, qword [boot_info]
    mov rsi, qword [version]

    xor rbp, rbp
    xor rsp, rsp
    mov rsp, qword [stack]
    push qword 0                                ; Push an invalid return address

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

boot_info: dq 0
version: dq 0
top_page_table: dw 0
stack: dq 0
kernel_entry: dq 0
