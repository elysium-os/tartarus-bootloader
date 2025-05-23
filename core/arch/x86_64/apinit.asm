global g_apinit_start
global g_apinit_end

section .rodata

%define off(addr) ebx + (addr - g_apinit_start)
%define off_cs(addr) cs:(addr - g_apinit_start)

g_apinit_start:
bits 16
    cli
    cld

    mov ebx, cs
    shl ebx, 4                                              ; Calculate the offset of the loaded code (cs * 0x10)

    o32 lgdt [off_cs(boot_info.gdtr)]                       ; Load GDT

    lea eax, [off(.protected)]
    mov dword [off_cs(.far_jmp32)], eax                     ; Set far jump offset

    mov eax, cr0
    or eax, 1                                               ; Set protected mode bit
    mov cr0, eax

    o32 jmp far [off_cs(.far_jmp32)]                        ; Far jump to protected mode
.far_jmp32:
    dd 0
    dw 0x18                                                 ; Code32 selector

.protected:
bits 32
    mov eax, 0x20                                           ; Data32 selector
    mov ds, eax
    mov ss, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    xor ax, ax
    lldt ax                                                 ; Clear LDT

    lea eax, [off(.long)]
    mov dword [off(.far_jmp64)], eax                        ; Set far jump offset

    mov eax, [off(boot_info.pml4)]
    mov cr3, eax                                            ; Load CR3 with the pml4

    mov eax, cr4
    or eax, (1 << 5)                                        ; Set PAE bit
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)                                        ; Set long mode bit
    wrmsr

    mov eax, cr0
    or eax, (1 << 31) | (1 << 16)                           ; Set paging bit & write protect bits
    mov cr0, eax

    jmp far [off(.far_jmp64)]
.far_jmp64:
    dd 0
    dw 0x28                                                 ; Code64 selector

.long:
bits 64
    mov rsp, [off(boot_info.stack)]                         ; Set RSP to the 4k stack
    mov rbp, rsp

    cmp byte [off(boot_info.set_nx)], 0
    je .noxd
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 11)                                       ; Set NX bit
    wrmsr
.noxd:
    mov rax, 0x30                                           ; Data64 selector
    mov ds, rax
    mov ss, rax
    mov es, rax
    mov fs, rax
    mov gs, rax

    mov rdx, qword [off(boot_info.wait_on_address)]

    lock inc byte [off(boot_info.init)]                     ; Increment init to 1 to signal the BSP we are done
.loop:
    xor rax, rax
    lock xadd qword [rdx], rax
    jnz short .handoff
    jmp short .loop
.handoff:
    jmp qword [rdx]

g_apinit_end:

boot_info:
    .init: db 0
    .lapic_id: db 0
    .pml4: dd 0
    .wait_on_address: dq 0
    .stack: dq 0
    .gdtr:
        dw 0
        dd 0
    .set_nx: db 0
