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

ifdef FLAG

deploy: release .PHONY
	cp -r release/pc-bios release/qemu-system-x86_64 release/bzImage release/sandbox deploy/
	echo "$(FLAG)" > rootfs/flag
	$(MAKE) INIT="src/release/init" _fs
	cp rootfs.cpio.gz deploy/
	tar cvzf deploy.tar.gz deploy/

endif

release: qemu clear_fs .PHONY
	mkdir -p release/pc-bios
	echo 'FLAG OF FIRMWARE' > release/flag_firmware
	echo 'FLAG OF SANDBOX' > release/flag_sandbox
	echo 'FLAG OF KERNEL' > rootfs/flag
	$(MAKE) -C src/tests
	mkdir -p rootfs/home/chaos
	cp src/tests/test_ioctl rootfs/home/chaos/run
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
	mkdir -p rootfs/home/chaos
	cp src/tests/test_ioctl rootfs/home/chaos/run
	$(MAKE) INIT=src/tests/init build run

sol_kernel: .PHONY
	$(MAKE) -C solution kernel
	$(MAKE) _sol_build
	$(MAKE) FW=solution/build/firmware.bin.signed INIT=src/release/init build run

sol_firmware: .PHONY
	$(MAKE) -C solution firmware
	$(MAKE) _sol_build
	$(MAKE) FW=solution/build/firmware.bin.signed INIT=src/release/init build run

sol_sandbox: .PHONY
	$(MAKE) -C solution sandbox
	$(MAKE) _sol_build
	$(MAKE) FW=solution/build/firmware.bin.signed INIT=src/release/init build run

# make remote CHAL=kernel|firmware|sandbox
remote: .PHONY
	$(MAKE) -C solution $(CHAL)
	solution/send.rb

_sol_build: .PHONY
	mkdir -p rootfs/home/chaos
	cp solution/build/go rootfs/home/chaos/run

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
