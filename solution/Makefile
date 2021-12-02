BUILD_DIR=build

kernel: .PHONY
	$(MAKE) SRC_DIR=kernel _build

firmware: .PHONY
	$(MAKE) SRC_DIR=firmware _build

sandbox: .PHONY
	$(MAKE) SRC_DIR=sandbox _build
	# let our firmware has a fixed size to fix the heap layout
	python3 -c 'f="build/firmware.bin";c=open(f,"rb").read();open(f,"wb").write(c.ljust(0x1000,b"\x00"))'
	firmware/sign.py $(BUILD_DIR)/firmware.bin $(BUILD_DIR)/firmware.bin.signed

ifdef SRC_DIR

_build: .PHONY
	$(RM) -r $(BUILD_DIR)
	mkdir $(BUILD_DIR)
	cp -r ../src/chaos/firmware/* $(BUILD_DIR)/
	$(RM) $(BUILD_DIR)/*.{o,elf,bin,signed}
	$(CC) $(SRC_DIR)/exp.c -o $(BUILD_DIR)/go -static -I../src/linux/drivers/misc/chaos/
	cp $(SRC_DIR)/handler.c $(BUILD_DIR)/
	$(MAKE) -C $(BUILD_DIR)/
	firmware/sign.py $(BUILD_DIR)/firmware.bin $(BUILD_DIR)/firmware.bin.signed

endif

.PHONY:
