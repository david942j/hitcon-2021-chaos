FW ?= src/chaos/firmware/firmware.bin.signed
INIT ?= src/tests/init
QEMU_BUILD_DIR = qemu/build
QEMU_ROM_FILES = bios-256k.bin kvmvapic.bin linuxboot_dma.bin vgabios-stdvga.bin efi-e1000.rom

all: build run

run: .PHONY
	$(QEMU_BUILD_DIR)/qemu-system-x86_64 -kernel linux/arch/x86_64/boot/bzImage \
		-device chaos,sandbox=./src/chaos/sandbox \
		-initrd rootfs.cpio.gz -m 512 \
		-monitor none -nographic \
		-append "console=ttyS0 kaslr panic=1" -no-reboot

build: qemu kernel

clear_fs: .PHONY
	$(RM) -r rootfs
	tar xzf rootfs_clear.tar.gz

release: qemu clear_fs .PHONY
	mkdir -p release/pc-bios
	echo 'FLAG OF FIRMWARE' > release/flag_firmware
	echo 'FLAG OF SANDBOX' > release/flag_sandbox
	echo 'FLAG OF KERNEL' > rootfs/flag && chmod 400 rootfs/flag
	$(MAKE) INIT="src/release/init" kernel
	cp $(QEMU_BUILD_DIR)/qemu-system-x86_64 linux/arch/x86_64/boot/bzImage src/chaos/sandbox rootfs.cpio.gz release/
	$(foreach f,$(QEMU_ROM_FILES),cp $(QEMU_BUILD_DIR)/pc-bios/$(f) release/pc-bios/;)

qemu: .PHONY
	$(MAKE) -C src/chaos sandbox
	$(MAKE) -C src copy_qemu_src
	$(MAKE) -C qemu -j `nproc`

kernel: .PHONY
	$(MAKE) -C linux -j `nproc`
	$(MAKE) -C src driver
	cp src/linux/drivers/misc/chaos/chaos.ko rootfs/
	$(MAKE) FW="$(FW)" INIT="$(INIT)" _fs

test: .PHONY
	$(MAKE) -C src/tests
	cp -r src/tests rootfs/
	$(MAKE) INIT=src/tests/init build run

sol_kernel: .PHONY
	$(MAKE) -C solution kernel
	mkdir -p rootfs/tests
	cp solution/kernel/go rootfs/tests/
	# TODO: use release/init
	$(MAKE) FW=solution/firmware/firmware.bin.signed INIT=src/tests/init build run

# Do NOT depend on me - always use $(MAKE) INIT=... _fs
_fs: .PHONY
	$(MAKE) -C src/chaos firmware
	mkdir -p rootfs/lib/firmware
	cp $(FW) rootfs/lib/firmware/chaos
	cp $(INIT) rootfs/init && chmod +x rootfs/init
	cd rootfs && find . | cpio -o -Hnewc | gzip -9 > ../rootfs.cpio.gz

# only need to be run once after submodules being cloned
prepare: .PHONY
	cd qemu && ./configure --target-list=x86_64-softmmu
	$(MAKE) -C linux defconfig

.PHONY:
