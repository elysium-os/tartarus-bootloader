include conf.mk

.PHONY: all install

ifeq ($(PLATFORM), x86_64-bios)
all: $(BUILD_DIRECTORY)/core/tartarus.sys $(BUILD_DIRECTORY)/x86_64-bios.bin

$(BUILD_DIRECTORY)/x86_64-bios.bin:
	$(MAKE) -C $(SRC_DIRECTORY)/boot $@

$(BUILD_DIRECTORY)/core/tartarus.sys:
	$(MAKE) -C $(SRC_DIRECTORY)/core $@
endif

ifeq ($(PLATFORM), x86_64-uefi)

endif