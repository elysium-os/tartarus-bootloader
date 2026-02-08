local ld = require("ld")
local c = require("lang_c")
local nasm = require("lang_nasm")

local install = {}

-- Options
local options = {
    platform = fab.option("platform", { "x86_64-uefi", "x86_64-bios" }) or "x86_64-uefi",
    build_type = fab.option("buildtype", { "debug", "release" }) or "release"
}

-- Tools
local cc = c.get_clang()
assert(cc ~= nil, "No viable C compiler found")

local linker = ld.get_linker()
assert(linker ~= nil, "No viable linker found")

-- Rules
local objcopy_path = fab.which("llvm-objcopy") or fab.which("objcopy")
assert(objcopy_path ~= nil, "No viable objcopy found")

local objcopy_rule = fab.def_rule(
    "objcopy_to_bin",
    objcopy_path .. " -O binary @IN@ @OUT@",
    "Objcopying @IN@ to a binary @OUT@"
)

-- Common Dependencies
local freestanding_c_headers = fab.git(
    "freestanding-c-headers",
    "https://github.com/osdev0/freestnd-c-hdrs.git",
    "5e11c3da645d8f203e93dc23703b14a15c5b7afc"
)

local cc_runtime = fab.git(
    "cc-runtime",
    "https://github.com/osdev0/cc-runtime.git",
    "dae79833b57a01b9fd3e359ee31def69f5ae899b"
)

-- Common
local core_sources = sources(
    fab.glob("core/**/*.c", "!core/arch/**"),
    path(fab.build_dir(), cc_runtime.path, "src/cc-runtime.c")
)

local include_dirs = {
    c.include_dir("."),
    c.include_dir("core")
}

local cflags = {
    "-std=gnu2x",
    "-ffreestanding",
    "-nostdinc",

    "-fno-stack-protector",
    "-fno-stack-check",
    "-fno-omit-frame-pointer",
    "-fno-strict-aliasing",
    "-fno-lto",

    "-Wall",
    "-Wextra",
    "-Wvla",
    "-Wshadow",
    "-Werror"
}

local defines = {}

-- Build Types
if options.build_type == "debug" then
    table.insert(cflags, "-O3")
    table.insert(defines, "__BUILD_DEBUG")
end

if options.build_type == "release" then
    table.extend(cflags, { "-O0", "-g" })
    table.insert(defines, "__BUILD_RELEASE")
end

-- Platforms
if options.platform:starts_with("x86_64") then
    table.insert(defines, "__ARCH_X86_64")

    table.insert(include_dirs, c.include_dir(path(fab.build_dir(), freestanding_c_headers.path, "x86_64/include")))

    table.extend(cflags, {
        "-mabi=sysv",
        "-mgeneral-regs-only",
    })

    local asm_flags = {
        "-Werror"
    }

    local ld_flags = {
        "-znoexecstack",
        "-zcommon-page-size=0x1000",
        "-zmax-page-size=0x1000"
    }

    local asmc = nasm.get_nasm()
    if asmc == nil then
        error("No NASM assembler found")
    end

    local linker_script = nil

    if options.platform == "x86_64-uefi" then
        table.extend(core_sources,
            sources(fab.glob("core/arch/{x86_64,uefi}/**/*.{c,asm}", "!core/arch/x86_64/bios/**")))

        table.extend(cflags, {
            "--target=x86_64-none-elf",
            "-m64",
            "-fpie",
            "-march=x86-64",
            "-mno-red-zone",
            "-funsigned-char",
            "-fshort-wchar",
            "-DGNU_EFI_USE_MS_ABI"
        })

        table.extend(defines, {
            "__PLATFORM_X86_64_UEFI",
            "__UEFI",
        })

        table.extend(asm_flags, { "-f", "elf64" })

        table.extend(ld_flags, {
            "-melf_x86_64",
            "-ztext",
            "-pie"
        })

        -- Pico EFI
        local pico_efi = fab.git(
            "pico-efi",
            "https://codeberg.org/PicoEFI/PicoEFI.git",
            "8b79fdaa72ee548a8ea24e3dc4d87bf281312865"
        )

        table.extend(core_sources, sources(
            path(fab.build_dir(), pico_efi.path, "x86_64/reloc.c"),
            path(fab.build_dir(), pico_efi.path, "x86_64/entry.S")
        ))
        table.insert(include_dirs, c.include_dir(path(fab.build_dir(), pico_efi.path, "inc")))

        linker_script = fab.def_source(path(fab.build_dir(), pico_efi.path, "x86_64/link_script.lds"))
    end

    if options.platform == "x86_64-bios" then
        table.extend(core_sources,
            sources(fab.glob("core/arch/x86_64/**/*.{c,asm}", "!core/arch/x86_64/uefi/**")))

        table.extend(cflags, {
            "--target=x86_64-none-elf",
            "-m32",
            "-march=i686",
            "-fno-PIC",
            "-D__TARTARUS_NO_PTR"
        })

        table.insert(defines, "__PLATFORM_X86_64_BIOS")

        table.extend(asm_flags, {
            "-f", "elf32"
        })

        table.extend(ld_flags, {
            "-melf_i386",
        })

        linker_script = fab.def_source("core/arch/x86_64/bios/tartarus.ld")
    end

    assert(linker_script ~= nil)

    for _, define in ipairs(defines) do
        table.insert(cflags, "-D" .. define)
        table.insert(asm_flags, "-D" .. define)
    end

    local core_objects = generate(core_sources, {
        S = function(sources) return cc:generate(sources, cflags, include_dirs) end,
        c = function(sources) return cc:generate(sources, cflags, include_dirs) end,
        asm = function(sources) return asmc:generate(sources, asm_flags) end
    })

    local core = linker:link("tartarus.elf", core_objects, ld_flags, linker_script)
    local binary = objcopy_rule:build("tartarus.bin", { core }, {})

    if options.platform == "x86_64-bios" then
        local bios_boot = asmc:assemble("x86_64-bios.bin", fab.def_source("boot/x86_64-bios.asm"), { "-f", "bin" })

        install["share/tartarus/x86_64-bios.bin"] = bios_boot
        install["share/tartarus/tartarus.sys"] = binary
    end

    if options.platform == "x86_64-uefi" then
        local efi = fab.def_rule(
            "postprocess_efi",
            fab.path_rel("uefi_postprocess.sh") .. " @IN@ @OUT@",
            "Postprocessing @IN@ to @OUT@"
        ):build("tartarus.efi", { binary }, {})

        install["share/tartarus/tartarus.efi"] = efi
    end

end

return { install = install }
