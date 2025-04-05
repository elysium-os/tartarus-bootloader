if get_option('platform') == 'x86_64-bios'
    nasm = find_program('nasm')

    custom_target(
        'x86_64-bios.bin',
        command : [ nasm, '@INPUT@', '-f', 'bin', '-o', '@OUTPUT@' ],
        input : files('x86_64-bios.asm'),
        output : 'x86_64-bios.bin',
        install_dir : 'share/tartarus',
        install : true
    )
endif
