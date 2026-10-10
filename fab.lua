local ld = require("ld")
local c = require("lang_c")
local nasm = require("lang_nasm")

-- Options
local options = {
    platform = fab.option("platform", {
        "x86_64-uefi",
        "x86_64-bios",
        "aarch64-uefi",
        "riscv64-opensbi",
        "riscv64-uefi",
    }) or "x86_64-uefi" --[[@as string]],

    build_type = fab.option("buildtype", { "debug", "release" }) or "release" --[[@as string]],
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
    "https://github.com/osdev0/freestanding-c-hdrs.git",
    "38fed4e1e3365733ddbfa03b0a28936243ad31e9"
)

local cc_runtime = fab.git(
    "cc-runtime",
    "https://github.com/osdev0/cc-runtime.git",
    "9891629daf3703d0ddecd295772df7da82ab25b9"
)

local smoldtb = fab.git(
    "smoldtb",
    "https://github.com/DeanoBurrito/smoldtb.git",
    "72d3f8596fb91244e646944e09dcd93e1be03140"
)

-- Common
local core_sources = sources(
    fab.glob("core/**/*.c", "!core/arch/**"),
    path(fab.build_dir(), cc_runtime.path, "src/cc-runtime.c")
)

local smoldtb_sources = sources(path(fab.build_dir(), smoldtb.path, "smoldtb.c"))

local include_dirs = {
    c.include_dir("."),
    c.include_dir("core"),
    c.include_dir(path(fab.build_dir(), freestanding_c_headers.path, "include")),
    c.include_dir(path(fab.build_dir(), smoldtb.path))
}

local flags = {
    c = {
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
    },
    ld = {}
}

local defines = {}

local linker_script = nil

-- Build Types
if options.build_type == "debug" then
    table.insert(flags.c, "-O3")
    table.insert(defines, "__BUILD_DEBUG")
end

if options.build_type == "release" then
    table.extend(flags.c, { "-O0", "-g" })
    table.insert(defines, "__BUILD_RELEASE")
end

-- Platforms
local architecture = options.platform:split("-")[1]
local firmware = options.platform:split("-")[2]

if architecture == "x86_64" then
    table.insert(defines, "__ARCH_X86_64")

    table.extend(flags.c, {
        "-mabi=sysv",
        "-mgeneral-regs-only",
    })

    flags.asm = {
        "-Werror"
    }

    table.extend(flags.ld, {
        "-znoexecstack",
        "-zcommon-page-size=0x1000",
        "-zmax-page-size=0x1000"
    })

    table.extend(core_sources, sources(fab.glob("core/arch/x86_64/**/*.{c,asm}", "!core/arch/x86_64/{uefi,bios}/**")))

    if firmware == "bios" then
        table.insert(defines, "__PLATFORM_X86_64_BIOS")

        table.extend(flags.c, {
            "--target=x86_64-none-elf",
            "-m32",
            "-march=i686",
            "-fno-PIC",
            "-D__TARTARUS_NO_PTR"
        })

        table.extend(flags.asm, {
            "-f", "elf32"
        })

        table.extend(flags.ld, {
            "-melf_i386",
        })

        table.extend(core_sources, sources(fab.glob("core/arch/x86_64/bios/**/*.{c,asm}")))

        linker_script = fab.def_source("core/arch/x86_64/bios/tartarus.ld")
    end

    if firmware == "uefi" then
        table.insert(defines, "__PLATFORM_X86_64_UEFI")

        table.extend(flags.c, {
            "--target=x86_64-none-elf",
            "-m64",
            "-fpie",
            "-march=x86-64",
            "-mno-red-zone",
            "-funsigned-char",
            "-fshort-wchar",
            "-DGNU_EFI_USE_MS_ABI"
        })

        table.extend(flags.asm, { "-f", "elf64" })

        table.extend(flags.ld, {
            "-melf_x86_64",
            "-ztext",
            "-pie"
        })

        table.extend(core_sources, sources(fab.glob("core/arch/x86_64/uefi/**/*.{c,asm}")))
    end
end

if architecture == "aarch64" then
    table.insert(defines, "__ARCH_AARCH64")

    table.extend(flags.c, {
        "-target aarch64-unknown-none-elf",
        "-mcpu=generic",
        "-march=armv8-a+nofp+nosimd",
        "-mgeneral-regs-only",
        "-fpie",
        "-fshort-wchar",
        "-funsigned-char",
    })

    table.extend(flags.ld, {
        "-maarch64elf",
        "-ztext",
        "-pie"
    })

    table.extend(core_sources, sources(fab.glob("core/arch/aarch64/**/*.{c,S}")))

    if firmware == "uefi" then
        table.insert(defines, "__PLATFORM_AARCH64_UEFI")
    end
end

if architecture == "riscv64" then
    table.insert(defines, "__ARCH_RISCV64")

    table.extend(flags.c, {
        "-target riscv64-unknown-none-elf",
        "-mabi=lp64",
        "-march=rv64ima_zihintpause",
        "-mcmodel=medany",
        "-fshort-wchar",
        "-funsigned-char",
    })

    table.extend(flags.ld, {
        "--no-relax-gp",
        "-melf64lriscv",
        "-ztext",
    })

    table.extend(core_sources, sources(fab.glob("core/arch/riscv64/**/*.{c,S}", "!core/arch/riscv64/{uefi,opensbi}/**")))

    if firmware == "opensbi" then
        table.insert(flags.c, "-fno-pic")

        table.insert(defines, "__PLATFORM_RISCV64_OPENSBI")
        table.extend(core_sources, sources(fab.glob("core/arch/riscv64/opensbi/**/*.{c,S}")))
        linker_script = fab.def_source("core/arch/riscv64/opensbi/tartarus.ld")
    end

    if firmware == "uefi" then
        table.insert(flags.c, "-fpie")
        table.insert(flags.ld, "-pie")

        table.insert(defines, "__PLATFORM_RISCV64_UEFI")
    end
end

if firmware == "uefi" then
    table.insert(defines, "__UEFI")

    table.extend(core_sources, sources(fab.glob("core/arch/uefi/**/*.c")))

    local pico_efi = fab.git(
        "pico-efi",
        "https://codeberg.org/PicoEFI/PicoEFI.git",
        "8b79fdaa72ee548a8ea24e3dc4d87bf281312865"
    )

    table.insert(include_dirs, c.include_dir(path(fab.build_dir(), pico_efi.path, "inc")))

    table.extend(core_sources, sources(
        path(fab.build_dir(), pico_efi.path, architecture, "reloc.c"),
        path(fab.build_dir(), pico_efi.path, architecture, "entry.S")
    ))

    linker_script = fab.def_source(path(fab.build_dir(), pico_efi.path, architecture, "link_script.lds"))
end

for _, define in ipairs(defines) do
    table.insert(flags.c, "-D" .. define)

    if architecture == "x86_64" then
        table.insert(flags.asm, "-D" .. define)
    end
end

-- Build
assert(linker_script ~= nil)

local install = {}

local generators = {
    S = function(sources) return cc:generate(sources, flags.c, include_dirs) end,
    c = function(sources) return cc:generate(sources, flags.c, include_dirs) end,
}

if architecture == "x86_64" then
    local asmc = nasm.get_nasm()
    if asmc == nil then
        error("No NASM assembler found")
    end

    generators.asm = function(sources) return asmc:generate(sources, flags.asm) end

    if firmware == "bios" then
        install["share/tartarus/x86_64-bios.bin"] = asmc:assemble("x86_64-bios.bin", fab.def_source("boot/x86_64-bios.asm"), { "-f", "bin" })
    end
end

local smoldtb_cflags = {}
table.extend(smoldtb_cflags, flags.c)
table.extend(smoldtb_cflags, {
    "-Wno-unused-function",
    "-Wno-tautological-overlap-compare",
})

local core_objects = generate(core_sources, generators)
table.extend(core_objects, cc:generate(smoldtb_sources, smoldtb_cflags, include_dirs))

local core = linker:link("tartarus.elf", core_objects, flags.ld, linker_script)
local binary = objcopy_rule:build("tartarus.bin", { core }, {})

if options.platform == "x86_64-bios" then
    install["share/tartarus/tartarus.sys"] = binary
end

if firmware == "opensbi" then
    install["share/tartarus/" .. architecture .. "-opensbi.bin"] = binary
end

if firmware == "uefi" then
    install["share/tartarus/tartarus.efi"] = fab.def_rule(
        "postprocess_efi",
        fab.path_rel("scripts/pad.sh") .. " @IN@ @OUT@ 4096",
        "Postprocessing @IN@ to @OUT@"
    ):build("tartarus.efi", { binary }, {})
end

return { install = install }
