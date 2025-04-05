asm_sources = files(
    'apinit.asm',
    'mem.asm'
)

sources += files(
    'cpu.c',
    'gdt.c',
    'lapic.c',
    'smp.c',
    'tsc.c',
    'vm.c'
)

cflags += [
    '-mabi=sysv',
    '-mgeneral-regs-only'
]

ldflags += [
    '-Wl,-znoexecstack',
    '-Wl,-zcommon-page-size=0x1000',
    '-Wl,-zmax-page-size=0x1000'
]

if get_option('platform') == 'x86_64-bios'
    subdir('bios')
endif

if get_option('platform') == 'x86_64-uefi'
    subdir('uefi')
endif
