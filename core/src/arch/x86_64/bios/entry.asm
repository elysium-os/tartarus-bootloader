extern x86_64_bios_entry
extern g_x86_64_gdt
extern g_x86_64_gdt_limit
extern a20_enable

bits 16
section .entry
    jmp entry_real

section .early
entry_real:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Test for CPUID
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    xor eax, ecx
    jz error.no_cpuid

    ; Test for CPUID extended info
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb error.no_cpuid_ext

    ; Test for long mode
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz error.long_mode_unsupported

    ; Enable the a20 line
    call a20_enable
    jnc error.couldnt_enable_a20

    cli
    push dword g_x86_64_gdt
    push word [g_x86_64_gdt_limit]
    lgdt [esp]
    add esp, 6

    ; Set protected mode bit
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x18:entry_protected

bits 32
entry_protected:
    mov eax, 0x20
    mov ds, eax
    mov ss, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    mov bp, sp

    jmp x86_64_bios_entry

bits 16
error:
.no_cpuid:
    mov si, ERROR_NO_CPUID
    jmp .print
.no_cpuid_ext:
    mov si, ERROR_NO_CPUID_EXT
    jmp .print
.long_mode_unsupported:
    mov si, ERROR_NO_LONG_MODE
    jmp .print
.couldnt_enable_a20:
    mov si, ERROR_A20_FAIL
    jmp .print

.print:
    mov ah, 0xE
    xor bx, bx
.loop:
    mov al, byte [si]
    cmp al, 0
    jz .done
    int 0x10
    inc si
    jmp .loop
.done:
    cli
    hlt

ERROR_NO_CPUID: db "Early Error: CPUID not supported.", 0
ERROR_NO_CPUID_EXT: db "Early Error: CPUID extended info not supported.", 0
ERROR_NO_LONG_MODE: db "Early Error: Long mode not supported.", 0
ERROR_A20_FAIL: db "Early Error: Failed to enable A20.", 0