run: fs qemu kernel
	qemu/build/qemu-system-x86_64 -kernel linux/arch/x86_64/boot/bzImage \
		-initrd rootfs.cpio.gz -m 512 \
		-monitor none -nographic \
		-append "console=ttyS0 kaslr panic=-1" -no-reboot

qemu: .PHONY
	$(MAKE) -C qemu -j `nproc`

kernel: .PHONY
	$(MAKE) -C linux -j `nproc`

fs: .PHONY
	cd rootfs && find . | cpio -o -Hnewc | gzip -9 > ../rootfs.cpio.gz

.PHONY:
