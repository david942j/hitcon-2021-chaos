DEPS = $(wildcard *.h)
all: firmware

firmware: entry.o handler.o syscall.S
	$(CC) $^ -Os -nostdlib -o firmware.elf -T linker.ld
	objcopy -O binary firmware.elf firmware.bin
	./sign.py firmware.bin firmware.bin.signed

%.o: %.c $(DEPS)
	$(CC) -Os -nostdlib -c -o $@ $<
