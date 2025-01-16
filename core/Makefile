# Sources
C_SOURCES := $(shell find src -type f -regex "src\/[^.]*?.c")

ifneq (, $(filter $(PLATFORM), x86_64-bios x86_64-uefi))
    C_SOURCES += $(shell find src -type f -name "*.x86_64.c")
endif

ifeq ($(PLATFORM), x86_64-bios)
    C_SOURCES += $(shell find src -type f -name "*.x86_64.bios.c")
endif

ifeq ($(PLATFORM), x86_64-uefi)
    C_SOURCES += $(shell find src -type f -name "*.x86_64.uefi.c")
endif

C_OBJ := $(patsubst %.c,$(BUILD_DIRECTORY)/core/%.o,$(C_SOURCES))

# Flags
C_FLAGS := -std=gnu2x -ffreestanding
C_FLAGS += -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing -fno-lto
C_FLAGS += -Wall -Wextra -Wvla -Wshadow
C_FLAGS += -I$(SRC_DIRECTORY)
C_FLAGS += -D__ENV_$(shell echo $(ENVIRONMENT) | tr a-z A-Z) -D__PLATFORM_$(shell echo $(PLATFORM) | tr a-z A-Z | tr - _)
LD_FLAGS := -nostdlib

ifeq ($(ENVIRONMENT), production)
    C_FLAGS += -O3
endif

ifeq ($(ENVIRONMENT), development)
    C_FLAGS += -g
endif

ifneq (, $(filter $(PLATFORM), x86_64-bios x86_64-uefi))
    C_FLAGS += -D__ARCH_X86_64
    C_FLAGS += -mabi=sysv -mgeneral-regs-only
    LD_FLAGS += -z common-page-size=0x1000 -z max-page-size=0x1000
endif

ifeq ($(PLATFORM), x86_64-bios)
    C_FLAGS += -m32 -march=i686 -fno-PIC
    LD_FLAGS += -m elf_i386 -static
endif

ifeq ($(PLATFORM), x86_64-uefi)
    C_FLAGS += -m64 -march=x86-64 -mno-red-zone -fPIE -funsigned-char -fshort-wchar
    C_FLAGS += -D GNU_EFI_USE_MS_ABI -I$(EFIDIR)/inc -I$(EFIDIR)/inc/x86_64
    LD_FLAGS += -m elf_x86_64 -pie -z text
endif

# Wildcard Targets
$(BUILD_DIRECTORY)/core/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(C_FLAGS) -o $@ -c $<

# Final Targets
ifeq ($(PLATFORM), x86_64-bios)
$(BUILD_DIRECTORY)/core/tartarus.sys: $(BUILD_DIRECTORY)/core/tartarus.elf
	mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

$(BUILD_DIRECTORY)/core/tartarus.elf: $(C_OBJ) $(VENDOR_LIBGCC)/libgcc-i686.a
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -T"$(SRC_DIRECTORY)/core/support/link.x86_64.bios.ld" -o $@ $^
endif

ifeq ($(TARGET), x86_64-uefi)
$(BUILD_DIRECTORY)/core/tartarus.efi: $(BUILD_DIRECTORY)/core/tartarus.elf
	mkdir -p $(@D)
	$(OBJCOPY) -O binary $< $@
	dd if=/dev/zero of='$@' bs=4096 count=0 seek=$$(( ($$(wc -c < '$@') + 4095) / 4096 ))
	chmod -x $@

$(BUILD_DIRECTORY)/core/tartarus.elf: $(C_OBJ) $(VENDOR_LIBGCC)/libgcc-x86_64-no-red-zone.a $(VENDOR_EFI)/src/crt0-efi-x86_64.S.o $(VENDOR_EFI)/src/reloc_x86_64.c.o
	mkdir -p $(@D)
	$(LD) $(LD_FLAGS) -T"$(VENDOR_EFI)/src/elf_x86_64_efi.lds" -o $@ $^

$(VENDOR_EFI)/src/crt0-efi-x86_64.S.o $(VENDOR_EFI)/src/reloc_x86_64.c.o:
	$(MAKE) ARCH=x86_64 CC="$(CC)" CFLAGS="$(C_FLAGS)" -C $(VENDOR_EFI)/src -f nyu-efi.mk
endif
