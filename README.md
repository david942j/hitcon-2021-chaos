# hitcon-2021-chaos

## Challenge Descriptions

Let's introduce our brand new device for this cryptocurrency era - CHAOS the CryptograpHy AcceleratOr Silicon!

Remote Env: Ubuntu 20.04

`nc 52.197.161.60 3154`

chaos-7e0d17f7553a86831ec6f1a5aba6bdb8cfab5674.tar.gz

### chaos-firmware

Author: david942j

10 Teams solved.

### chaos-kernel

Hint: solve chaos-firmware first

Author: david942j

3 Teams solved.

### chaos-sandbox

Hint: solve chaos-firmware first

Author: david942j,lyc

2 Team solved.

## Writeups

* [Official writeup](https://david942j.blogspot.com/2021/12/official-write-up-hitcon-ctf-2021-chaos.html
)
* [Writeup](https://org.anize.rs/HITCON-2021/pwn/chaos) by team [organizers](https://ctftime.org/team/42934)

## Developer Notes

Do these once after cloned.
```
$ sudo apt install libelf-dev ninja-build
$ git submodule update --init
$ make prepare
```

### Integration Tests

```
$ make test
```
