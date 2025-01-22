include conf.mk

DESTDIR ?= /

.PHONY: all install

ifeq ($(PLATFORM), x86_64-bios)
all: $(BUILD_DIRECTORY)/core/tartarus.sys $(BUILD_DIRECTORY)/x86_64-bios.bin

$(BUILD_DIRECTORY)/x86_64-bios.bin:
	$(MAKE) -C $(SRC_DIRECTORY)/boot $@

$(BUILD_DIRECTORY)/core/tartarus.sys:
	$(MAKE) -C $(SRC_DIRECTORY)/core $@

install:
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/share/tartarus
	install $(SRC_DIRECTORY)/tartarus.h $(DESTDIR)$(PREFIX)/include
	install $(BUILD_DIRECTORY)/x86_64-bios.bin $(BUILD_DIRECTORY)/core/tartarus.sys $(DESTDIR)$(PREFIX)/share/tartarus
endif

ifeq ($(PLATFORM), x86_64-uefi)

endif