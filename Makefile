run: fs qemu kernel
	qemu/build/qemu-system-x86_64 -kernel linux/arch/x86_64/boot/bzImage \
		-initrd rootfs.cpio.gz -m 512 \
		-monitor none -nographic \
		-append "console=ttyS0 kaslr panic=-1" -no-reboot

qemu: .PHONY
	$(MAKE) -C src copy_qemu_src
	$(MAKE) -C qemu -j `nproc`

kernel: .PHONY
	$(MAKE) -C linux -j `nproc`

fs: .PHONY
	cd rootfs && find . | cpio -o -Hnewc | gzip -9 > ../rootfs.cpio.gz

# only need to be run once after submodules being cloned
prepare: .PHONY
	cd qemu && ./configure --target-list=x86_64-softmmu
	$(MAKE) -C linux defconfig

.PHONY:
