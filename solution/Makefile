kernel: .PHONY
	$(CC) kernel/exp.c -o kernel/go -static -I../src/linux/drivers/misc/chaos/
	$(MAKE) SRC=kernel/handler.c _build_fw

_build_fw: .PHONY
	$(RM) -r firmware/
	cp -r ../src/chaos/firmware .
	cp $(SRC) firmware/
	$(MAKE) -C firmware/

.PHONY:
