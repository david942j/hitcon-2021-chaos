#!/bin/bash

./qemu-system-x86_64 \
  -kernel ./bzImage \
  -initrd ./rootfs.cpio.gz \
  -nographic \
  -monitor none \
  -cpu qemu64 \
  -append "console=ttyS0 kaslr panic=1" \
  -no-reboot \
  -m 512M \
  -device chaos,sandbox=./sandbox
