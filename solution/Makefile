kernel: .PHONY
	$(CC) kernel/exp.c -o kernel/go -static -I../src/linux/drivers/misc/chaos/
	$(MAKE) SRC=kernel/handler.c _build_fw

firmware: .PHONY
	$(CC) firmware/exp.c -o firmware/go -static -I../src/linux/drivers/misc/chaos/
	$(MAKE) SRC=firmware/handler.c _build_fw
	firmware/sign.py sol_firmware/firmware.bin sol_firmware/firmware.bin.signed

_build_fw: .PHONY
	$(RM) -r sol_firmware/
	cp -r ../src/chaos/firmware sol_firmware
	$(RM) sol_firmware/*.{o,elf,bin,signed}
	cp $(SRC) sol_firmware/
	$(MAKE) -C sol_firmware/

.PHONY:
