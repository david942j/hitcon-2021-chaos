#!/bin/bash

exec 2>/dev/null
timeout -s KILL 120 nsjail -Q --chroot / --cwd /home/chaos --user chaos --group chaos -- \
./qemu-system-x86_64 \
  -kernel ./bzImage \
  -initrd $1 \
  -nographic \
  -monitor none \
  -cpu qemu64 \
  -append "console=ttyS0 kaslr panic=1" \
  -no-reboot \
  -m 512M \
  -device chaos,sandbox=./sandbox
