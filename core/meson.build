add_languages('c', native : false)

sources = []
includes = []

sources += files('core.c')

includes += include_directories('.')
includes += tartarus_header

defines = [
    '-D__ARCH_' + arch.to_upper(),
    '-D__PLATFORM_' + get_option('platform').to_upper().replace('-', '_')
]

cflags = [
    '-std=gnu2x',
    '-ffreestanding',
    '-nostdinc',

    '-fno-stack-protector',
    '-fno-stack-check',
    '-fno-omit-frame-pointer',
    '-fno-strict-aliasing',
    '-fno-lto',

    '-Wall',
    '-Wextra',
    '-Wvla',
    '-Wshadow'
]

ldflags = [
    '-nostdlib'
]

if get_option('buildtype') == 'release'
    defines += [
        '-D__ENV_PRODUCTION'
    ]
    cflags += [
        '-O3'
    ]
endif

if get_option('buildtype') == 'debug'
    defines += [
        '-D__ENV_DEVELOPMENT'
    ]
    cflags += [
        '-O0',
        '-g'
    ]
endif

subdir('common')
subdir('dev')
subdir('fs')
subdir('lib')
subdir('memory')
subdir('protocol')
subdir('arch' / arch)
