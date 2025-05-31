-- Options
local opt_platform = fab.option("platform", { "x86_64-uefi", "x86_64-bios" }) or "x86_64-uefi"
local opt_build_type = fab.option("buildtype", { "development", "production" }) or "development"

-- Tools
local cc = builtins.c.get_compiler("clang")
if cc == nil then
    error("No viable C compiler found")
end

local linker = builtins.get_linker()
if linker == nil then
    error("No viable linker found")
end

-- Rules
local objcopy = builtins.resolve_executable({ "llvm-objcopy", "objcopy" })
if objcopy == nil then
    error("no viable objcopy found")
end

local objcopy_rule = fab.rule({
    name = "objcopy_to_bin",
    command = { objcopy, "-O", "binary", "@IN@", "@OUT@" },
    description = "Objcopying @IN@ to a binary @OUT@",
})

-- Common Dependencies
local freestanding_c_headers = fab.dependency(
    "freestanding-c-headers",
    "https://codeberg.org/osdev/freestnd-c-hdrs.git",
    "21b59ecd6ef67bb32f893da8288ce08a324d1986"
)

local cc_runtime = fab.dependency(
    "cc-runtime",
    "https://codeberg.org/osdev/cc-runtime.git",
    "d5425655388977fa12ff9b903e554a20b20c426e"
)

-- Common
local core_sources = sources(fab.glob("core/**/*.c", "core/arch/**"), path(cc_runtime.path, "cc-runtime.c"))

local include_dirs = { builtins.c.include_dir("."), builtins.c.include_dir("core") }

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
}

local defines = {}

-- Build Types
if opt_build_type == "development" then
    table.insert(cflags, "-O3")
    table.insert(defines, "__ENV_DEVELOPMENT")
end

if opt_build_type == "production" then
    table.extend(cflags, { "-O0", "-g" })
    table.insert(defines, "__ENV_PRODUCTION")
end

-- Platforms
if opt_platform:starts_with("x86_64") then
    table.insert(include_dirs, builtins.c.include_dir(path(freestanding_c_headers.path, "x86_64/include")))

    table.extend(cflags, {
        "-mabi=sysv",
        "-mgeneral-regs-only",
    })

    table.insert(defines, "__ARCH_X86_64")

    local asm_flags = {
        "-Werror"
    }

    local ld_flags = {
        "-znoexecstack",
        "-zcommon-page-size=0x1000",
        "-zmax-page-size=0x1000"
    }

    local asmc = builtins.nasm.get_assembler()
    if asmc == nil then
        error("No NASM assembler found")
    end

    if opt_platform == "x86_64-uefi" then
        table.extend(core_sources, sources(fab.glob("core/arch/x86_64/**/*.{c,asm}", "core/arch/x86_64/bios/**")))

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

        table.insert(defines, "__PLATFORM_X86_64_UEFI")

        table.extend(asm_flags, { "-f", "elf64" })

        table.extend(ld_flags, {
            "-melf_x86_64",
            "-ztext",
            "-pie"
        })

        -- Nyu EFI
        local nyu_efi = fab.dependency(
            "nyu-efi",
            "https://codeberg.org/osdev/nyu-efi.git",
            "trunk"
        )

        table.extend(core_sources, sources(
            path(nyu_efi.path, "x86_64/reloc.c"),
            path(nyu_efi.path, "x86_64/entry.S")
        ))
        table.insert(include_dirs, builtins.c.include_dir(path(nyu_efi.path, "inc")))
        table.insert(ld_flags, "-T" .. fab.path_rel(path(nyu_efi.path, "x86_64/link_script.lds")))
    end

    if opt_platform == "x86_64-bios" then
        table.extend(core_sources, sources(fab.glob("core/arch/x86_64/**/*.{c,asm}", "core/arch/x86_64/uefi/**")))

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
            "-T" .. fab.path_rel("core/support/link.x86_64.bios.ld")
        })
    end

    for _, define in ipairs(defines) do
        table.insert(cflags, "-D" .. define)
        table.insert(asm_flags, "-D" .. define)
    end

    local core_objects = builtins.generate(core_sources, {
        S = function(sources) return cc:generate(sources, cflags, include_dirs) end,
        c = function(sources) return cc:generate(sources, cflags, include_dirs) end,
        asm = function(sources) return asmc:generate(sources, asm_flags) end
    })

    local core = linker:link("tartarus.elf", core_objects, ld_flags)
    local binary = objcopy_rule:build("tartarus.bin", { core }, {})

    if opt_platform == "x86_64-bios" then
        local bios_boot = asmc:assemble("x86_64-bios.bin", fab.source("boot/x86_64-bios.asm"), { "-f", "bin" })

        bios_boot:install("share/tartarus/x86_64-bios.bin")
        binary:install("share/tartarus/tartarus.sys")
    end

    if opt_platform == "x86_64-uefi" then
        local bash = fab.find_executable("bash")
        if bash == nil then
            error("bash not found")
        end

        local truncate = fab.find_executable("truncate")
        if truncate == nil then
            error("truncate not found")
        end

        local wc = fab.find_executable("wc")
        if wc == nil then
            error("wc not found")
        end

        local cp = fab.find_executable("cp")
        if cp == nil then
            error("cp not found")
        end

        local efi = fab.rule({
            name = "pad_efi",
            command = { bash, "-c", "\"" .. cp.path .. " @IN@ @OUT@ && " .. truncate.path .. " -s $((($(" .. wc.path .. " -c < @OUT@)+4095)/4096*4))K @OUT@" .. "\"" },
            description = "Padding EFI @IN@ to @OUT@"
        }):build("tartarus.efi", { binary }, {})

        efi:install("share/tartarus/tartarus.efi")
    end
end
