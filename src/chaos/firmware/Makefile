all: firmware

firmware: entry.o handler.o syscall.S
	$(CC) $^ -Os -nostdlib -o firmware.elf -T linker.ld
	objcopy -O binary firmware.elf firmware.bin
	python3 -c "img=open('firmware.bin','rb').read();open('firmware.bin.signed','wb').write(__import__('struct').pack('I',len(img))+b'a'*128+img)"

%.o: %.c
	$(CC) -Os -nostdlib -c -o $@ $<
