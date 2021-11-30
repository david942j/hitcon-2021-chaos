BUILD_DIR=build
kernel: .PHONY
	$(CC) kernel/exp.c -o kernel/go -static -I../src/linux/drivers/misc/chaos/
	$(MAKE) SRC=kernel/handler.c _build_fw

firmware: .PHONY
	$(CC) firmware/exp.c -o firmware/go -static -I../src/linux/drivers/misc/chaos/
	$(MAKE) SRC=firmware/handler.c _build_fw

_build_fw: .PHONY
	$(RM) -r $(BUILD_DIR)
	mkdir $(BUILD_DIR)
	cp -r ../src/chaos/firmware/* $(BUILD_DIR)/
	$(RM) $(BUILD_DIR)/*.{o,elf,bin,signed}
	cp $(SRC) $(BUILD_DIR)/
	$(MAKE) -C $(BUILD_DIR)/
	firmware/sign.py $(BUILD_DIR)/firmware.bin $(BUILD_DIR)/firmware.bin.signed

.PHONY:
