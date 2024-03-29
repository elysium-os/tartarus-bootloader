include conf.mk

DESTDIR ?= /

.PHONY: all install

ifeq ($(TARGET), x86_64-bios)
all: $(BUILD)/x86_64-bios/mbr.bin $(BUILD)/core/tartarus.sys

$(BUILD)/x86_64-bios/mbr.bin:
	mkdir -p $(@D)
	$(MAKE) -C $(SRC)/boot $@

$(BUILD)/core/tartarus.sys:
	mkdir -p $(@D)
	$(MAKE) -C $(SRC)/core $@

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp $(SRC)/tartarus.h $(DESTDIR)$(PREFIX)/include
	mkdir -p $(DESTDIR)$(PREFIX)/share/tartarus
	cp $(BUILD)/x86_64-bios/mbr.bin $(BUILD)/core/tartarus.sys $(DESTDIR)$(PREFIX)/share/tartarus
endif

ifeq ($(TARGET), x86_64-uefi64)
all: $(BUILD)/core/tartarus.efi

$(BUILD)/core/tartarus.efi:
	mkdir -p $(@D)
	$(MAKE) -C $(SRC)/core $@

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp $(SRC)/tartarus.h $(DESTDIR)$(PREFIX)/include
	mkdir -p $(DESTDIR)$(PREFIX)/share/tartarus
	cp $(BUILD)/core/tartarus.efi $(DESTDIR)$(PREFIX)/share/tartarus
endif