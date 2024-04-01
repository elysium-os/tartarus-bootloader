# Expects TARGET, GIT, CC, LD, OBJCOPY, ASMC, LIBGCC_DIR, BUILD
EFIDIR := ../limine-efi

ifneq (, $(filter $(TARGET), x86_64-bios x86_64-uefi64))
    ARCH := x86_64
endif

# Sources
C_SOURCES := $(shell find src/ -type f -name "*.c" -not -path "src/hal/*")

ifeq ($(ARCH), x86_64)
    C_SOURCES += $(shell find src/hal/x86_64/ -type f -name "*.c")
    ASM_SOURCES := $(shell find src/hal/x86_64/ -type f -name "*.asm")
endif

ifeq ($(TARGET), x86_64-bios)
    C_SOURCES += $(shell find src/hal/x86_64-bios/ -type f -name "*.c")
    ASM_SOURCES += $(shell find src/hal/x86_64-bios/ -type f -name "*.asm")
endif

ifeq ($(TARGET), x86_64-uefi64)
    C_SOURCES += $(shell find src/hal/uefi/ -type f -name "*.c")
    ASM_SOURCES += $(shell find src/hal/uefi/ -type f -name "*.asm")
    C_SOURCES += $(shell find src/hal/x86_64-uef/ -type f -name "*.c")
    ASM_SOURCES += $(shell find src/hal/x86_64-uefi/ -type f -name "*.asm")
endif

ASM_OBJ := $(patsubst %.asm,$(BUILD)/core/%_asm.o,$(ASM_SOURCES))
C_OBJ := $(patsubst %.c,$(BUILD)/core/%.o,$(C_SOURCES))

# Flags
DEFINES :=
ASMFLAGS := -i$(SRC) -i$(SRC)/core/src
CFLAGS := -I$(SRC) -I$(SRC)/core/src -std=gnu2x -O2 -ffreestanding -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing -fno-lto --param min-pagesize=0
CFLAGS += -Wall -Wextra -Wshadow -Wvla -Wno-unused-variable
LDFLAGS := -nostdlib -static -z noexecstack

ifeq ($(ARCH), x86_64)
    DEFINES += -D __X86_64
    CFLAGS += -mabi=sysv -mgeneral-regs-only
    LDFLAGS += -z common-page-size=0x1000 -z max-page-size=0x1000
endif

ifeq ($(TARGET), x86_64-bios)
    DEFINES += -D __BIOS -D __TARTARUS_NO_PTR
    ASMFLAGS += -f elf32
    CFLAGS += -m32 -march=i686
endif

ifeq ($(TARGET), x86_64-uefi64)
    DEFINES += -D __UEFI -D __UEFI64
    ASMFLAGS += -f elf64
    CFLAGS += -m64 -march=x86-64 -mno-red-zone -D GNU_EFI_USE_MS_ABI -fPIE -funsigned-char -fshort-wchar -I$(EFIDIR)/inc -I$(EFIDIR)/inc/x86_64
    LDFLAGS += -pie --no-dynamic-linker -z text
endif

# Final Targets
ifeq ($(TARGET), x86_64-bios)
$(BUILD)/core/tartarus.sys: $(BUILD)/core/tartarus.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/core/tartarus.elf: $(C_OBJ) $(ASM_OBJ) $(LIBGCC_DIR)/i686.a
	$(LD) $(LDFLAGS) -T"link.x86_64.bios.ld" -o $@ $^
endif

ifeq ($(TARGET), x86_64-uefi64)
$(BUILD)/core/tartarus.efi: $(BUILD)/core/tartarus.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/core/tartarus.elf: $(EFIDIR)/gnuefi/crt0-efi-x86_64.S.o $(EFIDIR)/gnuefi/reloc_x86_64.c.o $(C_OBJ) $(ASM_OBJ) $(LIBGCC_DIR)/x86_64.a
	$(LD) $(LDFLAGS) -T"$(EFIDIR)/gnuefi/elf_x86_64_efi.lds" -o $@ $^

$(EFIDIR)/gnuefi/crt0-efi-x86_64.S.o $(EFIDIR)/gnuefi/reloc_x86_64.c.o: $(EFIDIR)
	$(MAKE) ARCH=x86_64 -C $(EFIDIR)/gnuefi
endif

# Wildcard Targets
$(BUILD)/core/%_asm.o: %.asm
	mkdir -p $(@D)
	$(ASMC) $(DEFINES) $(ASMFLAGS) -o $@ $<

$(BUILD)/core/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(DEFINES) $(CFLAGS) -o $@ -c $<

# Requirement Targets
$(EFIDIR):
	$(GIT) clone https://github.com/limine-bootloader/limine-efi.git $@ --depth=1