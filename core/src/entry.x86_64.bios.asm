extern entry
extern g_gdt
extern g_gdt_limit
extern a20_enable

bits 16
section .entry
    jmp entry_real

section .early
entry_real:
    xor ax, ax                              ; Reset all segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pushfd                                  ; Test for CPUID
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    xor eax, ecx
    jz error.no_cpuid

    mov eax, 0x80000000                     ; Test for CPUID extended info
    cpuid
    cmp eax, 0x80000001
    jb error.no_cpuid_ext

    mov eax, 0x80000001                     ; Test for long mode
    cpuid
    test edx, 1 << 29
    jz error.long_mode_unsupported

    call a20_enable                         ; Enable the a20 line
    jnc error.couldnt_enable_a20

    cli
    push dword g_gdt
    push word [g_gdt_limit]
    lgdt [esp]                              ; Load GDT
    add esp, 6

    mov eax, cr0
    or eax, 1                               ; Set protected mode bit
    mov cr0, eax

    jmp 0x18:entry_protected                ; Far jump into protected gdt segment

error:
.no_cpuid:
    mov bl, '1'
    jmp .print
.no_cpuid_ext:
    mov bl, '2'
    jmp .print
.long_mode_unsupported:
    mov bl, '3'
    jmp .print
.couldnt_enable_a20:
    mov bl, '4'
    jmp .print
.print:
    mov ax, (0xE << 8) | 'E'
    int 0x10
    mov al, '1'
    int 0x10
    mov al, bl
    int 0x10
    cli
    hlt

bits 32
entry_protected:
    mov eax, 0x20                           ; Reset all segments
    mov ds, eax
    mov ss, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    mov bp, sp

    jmp entry