$(BUILD_DIRECTORY)/%.bin: src/%.asm
	mkdir -p $(@D)
	$(NASM) $< -f bin -o $@