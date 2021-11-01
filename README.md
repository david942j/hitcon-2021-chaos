# hitcon-2021-chaos

## Developer Notes

`$ git submodule update --init`

### QEMU

```
$ cd qemu
$ ./configure --target-list=x86_64-softmmu
$ make -j`nproc`
$ file build/qemu-system-x86_64
```

### Linux

```
$ sudo apt install libelf-dev
$ make defconfig
$ make -j`nproc`
$ file arch/x86/boot/bzImage
```
