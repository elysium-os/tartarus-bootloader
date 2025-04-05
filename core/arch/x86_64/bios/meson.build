add_languages('nasm', native : false)

asm_sources += files(
    'protocol/linux.asm',
    'protocol/tartarus.asm',
    'entry.asm',
    'int.asm',
    'a20.asm'
)

sources += files(
    'acpi.c',
    'disk.c',
    'entry.c',
    'fb.c',
    'time.c'
)

cflags += [
    '--target=x86_64-none-elf',
    '-m32',
    '-march=i686',
    '-fno-PIC',
    '-D__TARTARUS_NO_PTR'
]

ldflags += [
    '-static',
    '-Wl,-melf_i386',
    '-Wl,-T' + meson.project_source_root() / 'core/support/link.x86_64.bios.ld'
]

tartarus_elf = executable(
    'tartarus.elf',
    dependencies : deps,
    sources : sources + asm_sources,
    include_directories : includes,
    c_args : cflags + defines,
    nasm_args : [ '-f', 'elf32', '-Werror' ],
    link_args : ldflags
)

objcopy = find_program('llvm-objcopy')

custom_target(
    'tartarus.sys',
    command : [ objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@' ],
    input : [ tartarus_elf ],
    output : 'tartarus.sys',
    install_dir : 'share/tartarus',
    install : true
)
