INIT ?= src/tests/init

all: build run

run: .PHONY
	qemu/build/qemu-system-x86_64 -kernel linux/arch/x86_64/boot/bzImage \
		-device chaos \
		-initrd rootfs.cpio.gz -m 512 \
		-monitor none -nographic \
		-append "console=ttyS0 kaslr panic=-1" -no-reboot

build: qemu kernel fs

qemu: .PHONY
	$(MAKE) -C src copy_qemu_src
	$(MAKE) -C qemu -j `nproc`

kernel: .PHONY
	$(MAKE) -C linux -j `nproc`
	$(MAKE) -C src driver
	cp src/linux/drivers/misc/chaos/chaos.ko rootfs/
	$(MAKE) INIT="$(INIT)" fs

fs: .PHONY
	cp $(INIT) rootfs/init && chmod +x rootfs/init
	cd rootfs && find . | cpio -o -Hnewc | gzip -9 > ../rootfs.cpio.gz

# only need to be run once after submodules being cloned
prepare: .PHONY
	cd qemu && ./configure --target-list=x86_64-softmmu
	$(MAKE) -C linux defconfig

.PHONY:
