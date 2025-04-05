asm_sources += files(
    'protocol/linux.asm',
    'protocol/tartarus.asm'
)

sources += files(
    'acpi.c',
    'disk.c',
    'uefi.c',
    'entry.c',
    'fb.c',
    'time.c'
)

cflags += [
    '--target=x86_64-none-elf',
    '-m64',
    '-march=x86-64',
    '-mno-red-zone',
    '-funsigned-char',
    '-fshort-wchar',
    '-DGNU_EFI_USE_MS_ABI'
]

ldflags += [
    '-Wl,-melf_x86_64',
    '-Wl,-ztext'
]

deps += dependency(
	'nyu-efi',
	required: true,
	fallback: ['nyu-efi', 'nyu_efi_deps']
)

# Compile nasm files like this because meson complains about
# the executable being PIE and nasm not "supporting" that.
nasm = find_program('nasm')

asm_objs = []
foreach asm_source : asm_sources
    asm_path = asm_source.full_path()
    asm_filename = asm_path.split('/')[-1]
    obj_name = asm_filename.split('.')[0] + '_obj'
    output_file = asm_filename.replace('.asm', '.o')

    obj = custom_target(
        obj_name,
        input: asm_source,
        output: output_file,
        command: [nasm, '-f', 'elf64', '-Werror', defines, '-o', '@OUTPUT@', '@INPUT@'],
        build_by_default: true
    )
    asm_objs += obj
endforeach

tartarus_elf = executable(
    'tartarus.elf',
    dependencies : deps,
    objects : asm_objs,
    sources : sources,
    include_directories : includes,
    c_args : cflags + defines,
    link_args : ldflags,
    pie : true
)

objcopy = find_program('llvm-objcopy')
bash = find_program('bash')
dd = find_program('dd')
wc = find_program('wc')

custom_target(
    'tartarus.efi',
    command : [
        objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@', '&&',
        bash, '-c', dd.full_path() + ' if=/dev/zero of=@OUTPUT@ bs=4096 count=0 seek=$((($(' + wc.full_path() + ' -c < @OUTPUT@)+4095)/4096))'
    ],
    input : [ tartarus_elf ],
    output : 'tartarus.efi',
    install_dir : 'share/tartarus',
    install : true
)
