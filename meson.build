project('Tartarus Bootloader', meson_version : '>=1.4.0')

deps = []

deps += dependency(
	'freestnd-c-hdrs-' + host_machine.cpu_family(),
	required: true,
	fallback: ['freestnd-c-hdrs', 'freestnd_c_hdrs_dep']
)

deps += dependency(
	'cc-runtime',
	required: true,
	fallback: ['cc-runtime', 'cc_runtime_deps']
)

tartarus_header = include_directories('.')

if get_option('platform') == 'x86_64-bios' or get_option('platform') == 'x86_64-uefi'
    arch = 'x86_64'
endif

subdir('boot')
subdir('core')
