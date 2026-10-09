OUTPUT_ARCH(riscv)
ENTRY(_start)

PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2));
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2));
    rodata  PT_LOAD FLAGS((1 << 2));
}

SECTIONS
{
    . = 0x80200000;

    ld_tartarus_start = .;

    .text : {
        *(.text.init)
        *(.text)
    } :text

    .rodata : {
        *(.rodata)
    } :rodata

    .data : {
        *(.data)
    } :data

    .bss : {
        _bss_start = .;
        *(.bss)
        *(COMMON)
        _bss_end = .;
    }

    . = ALIGN(0x1000);
    . += 0x10000;
    _stack_top = .;

    ld_tartarus_end = .;
}
