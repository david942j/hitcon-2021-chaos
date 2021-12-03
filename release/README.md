# CHAOS

CHAOS - CryptograpHy AcceleratOr Silicon

## Information

```
$ git submodule status
 a2547651bc896f95a3680a6a0a27401e7c7a1080 linux (v5.15.6)
 f9baca549e44791be0dd98de15add3d8452a8af0 qemu (v6.1.0)
```

```
$ cd qemu && ./configure --target-list=x86_64-softmmu && make
$ cd linux && make defconfig && make && make M=drivers/misc/chaos chaos.ko
```
