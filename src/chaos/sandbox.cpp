/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <sys/eventfd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/select.h>

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "buffer.h"
#include "check.h"
#include "crypto.h"
#include "inferior.h"
#include "seccomp.h"

namespace {

class MemoryRegion {
 public:
  MemoryRegion(uint64_t base, uint64_t size, int prot) : base_((void *)base), size_(size) {
    auto ptr = mmap(base_, size_, prot, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    CHECK(ptr == base_);
  }

  MemoryRegion(int fd, uint64_t base) : base_((void *)base) {
    lseek(fd, 0, SEEK_SET);
    size_ = lseek(fd, 0, SEEK_END);
    auto ptr = mmap(base_, size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    CHECK(ptr == base_);
    close(fd);
  }

  ~MemoryRegion() {
    munmap(base_, size_);
  }

  uint64_t Read64(uint64_t offset) const {
    CHECK(offset <= size_ - 8);
    return *static_cast<uint64_t *>(base_ + offset);
  }

  void Write64(uint64_t offset, uint64_t val) const {
    CHECK(offset <= size_ - 8);
    *static_cast<uint64_t *>(base_ + offset) = val;
  }

  void *at(uint64_t offset) const {
      CHECK(offset < size_);
      return base_ + offset;
  }

  inline uint64_t size() const { return size_; }
  inline void *base() const { return base_; }
  inline void *end() const { return base_ + size_; }

 private:
  void *base_;
  uint64_t size_;
};

class Event {
 public:
  Event(int fd) : fd_(fd) {
    CHECK(fd < FD_SETSIZE);
  }
  ~Event() {
    close(fd_);
  }

  void WaitAndClear() const {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd_, &readfds);
    int ret = select(fd_ + 1, &readfds, NULL, NULL, NULL);
    CHECK(ret == 1);
    uint64_t dummy;
    ret = read(fd_, &dummy, sizeof(dummy));
    CHECK(ret == sizeof(dummy));
  }

  void Trigger(uint64_t val = 1) const {
    int ret = write(fd_, &val, sizeof(val));
    CHECK(ret == sizeof(val));
  }

 private:
  int fd_;
};

constexpr int kCsrFd = 3;
constexpr int kDramFd = 4;
constexpr uint64_t kCsrBase   = 0x00010000;
constexpr uint64_t kDramBase  = 0x10000000;
constexpr uint64_t kCodeBase  = 0x00100000;
constexpr uint64_t kCodeSize  = 0x00100000; // 1MB
constexpr uint64_t kStackBase = 0x00201000;
constexpr uint64_t kStackSize = 0x00002000;

MemoryRegion CSR(kCsrFd, kCsrBase);
MemoryRegion DRAM(kDramFd, kDramBase);
MemoryRegion Code(kCodeBase, kCodeSize, PROT_READ | PROT_WRITE | PROT_EXEC);
MemoryRegion Stack(kStackBase, kStackSize, PROT_READ | PROT_WRITE);

int flag_firmware;
int flag_sandbox;

enum chaos_request_algo {
  /* copy input to output, for testing purpose */
  CHAOS_ALGO_ECHO,
  CHAOS_ALGO_MD5,
  CHAOS_ALGO_AES_ENC,
  CHAOS_ALGO_AES_DEC,
};

long HandleCryptoCall(Inferior &inferior, const uint64_t *args) {
  uint32_t in = args[1] >> 32;
  uint32_t in_size = args[1];
  uint32_t key = args[2] >> 32;
  uint32_t key_size = args[2];
  uint32_t out = args[3] >> 32;
  switch (args[0]) {
  case CHAOS_ALGO_MD5: {
    Buffer inb(in_size);
    if (!inb.FromUser(inferior, in))
      return -EFAULT;
    Buffer outb(crypto::MD5(inb));
    if (!outb.ToUser(inferior, out))
      return -EFAULT;
    return outb.size();
  }
  case CHAOS_ALGO_AES_ENC: {
    Buffer inb(in_size);
    if (!inb.FromUser(inferior, in))
      return -EFAULT;
    Buffer keyb(key_size);
    if (!keyb.FromUser(inferior, key))
      return -EFAULT;
    Buffer outb(crypto::AES_encrypt(keyb, inb));
    if (!outb.ToUser(inferior, out))
      return -EINVAL;
    return outb.size();
  }
  case CHAOS_ALGO_AES_DEC: {
    Buffer inb(in_size);
    if (!inb.FromUser(inferior, in))
      return -EFAULT;
    Buffer keyb(key_size);
    if (!keyb.FromUser(inferior, key))
      return -EFAULT;
    Buffer outb(crypto::AES_decrypt(keyb, inb));
    if (!outb.ToUser(inferior, out))
      return -EINVAL;
    return outb.size();
  }
  default:
    return -ENOSYS;
  }
}

long HandleFlagCall(Inferior &inferior, const uint64_t *args) {
  uint8_t buf[256] = {};
  ssize_t ret = read(flag_firmware, buf, sizeof(buf));
  if (ret <= 0)
    return ret;
  Buffer res(buf, ret);
  if (!res.ToUser(inferior, args[0]))
    return -EFAULT;
  return ret;
}

bool Sandboxing() {
  pid_t pid = fork();
  if (!pid) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    raise(SIGSTOP);
    // shouldn't reach here
    _exit(3);
  }

  Inferior inferior(pid);
  inferior.Attach();
  inferior.SetContext(Code.base(), Stack.end());
  while (1) {
    if (inferior.WaitForSys()) {
      if (inferior.IsSysCrpyto()) {
        auto res = HandleCryptoCall(inferior, inferior.sysargs());
        debug("HandleCryptoCall returned %ld\n", res);
        inferior.SetSyscallRet(res);
      } else if (inferior.IsSysFlag()) {
        inferior.SetSyscallRet(HandleFlagCall(inferior, inferior.sysargs()));
      } else if (inferior.IsExit()) {
        debug("firmware exited with %ld\n", inferior.sysargs()[0]);
        return inferior.sysargs()[0] == 0;
      }
    } else {
      debug("firmware crashed with 0x%x\n", inferior.lastStatus());
      return false;
    }
  }
}

void VerifyResult(int res) {
  CSR.Write64(0, (1ull << 63) | res);
}

constexpr int kMaxKeyLegnth = 255;
struct ImageHeader {
  uint32_t size; // image size without header
  uint8_t key_size;
  uint8_t key[kMaxKeyLegnth];
  uint8_t signature[kMaxKeyLegnth + 1];
  uint8_t code[];
} __attribute__((packed));

constexpr uint8_t kSignKeyLength = 128;
constexpr uint8_t kSignKey[kSignKeyLength] = {
  0x0f,0xff,0x0e,0xe9,0x45,0xbd,0x41,0x76,0xf5,0x5a,0x40,0x54,0x3b,0x36,0x66,0x84,0x3a,0x0d,0x56,0x5c,0x33,0x9e,0x5d,0x89,0x69,0xfc,0xd7,0xca,0x92,0x1c,0xc3,0x03,0xa1,0xc8,0xaf,0x16,0x24,0x0c,0x4d,0x03,0x2d,0x19,0x31,0x63,0x2b,0x90,0x99,0x6d,0xd4,0x8a,0xeb,0xac,0xee,0x30,0x7d,0x3c,0x57,0xbc,0x83,0x37,0x56,0x98,0xae,0x7d,0xf9,0x0d,0x10,0x16,0x3e,0xde,0xe9,0xe0,0x67,0xce,0x46,0xe7,0x38,0x09,0x22,0x57,0xda,0xfb,0x15,0xb8,0x0f,0xb6,0x59,0x61,0x90,0x0d,0xef,0xfa,0x9b,0x59,0xb5,0x7e,0x47,0x2b,0xf5,0x6b,0xe0,0xd9,0xf6,0x48,0xad,0x69,0x08,0xf2,0x55,0x3b,0xe1,0x3a,0x9e,0xa0,0xcd,0xa2,0x43,0x17,0x75,0x6c,0xba,0x51,0x42,0xa9,0x5e,0x21,0xf9,0xe0
};

void VerifyFirmware() {
  uint32_t off = CSR.Read64(0);
  uint32_t size = CSR.Read64(8);
  ImageHeader *image = reinterpret_cast<ImageHeader*>(DRAM.at(off));
  ImageHeader header;

  if (size <= sizeof(ImageHeader))
    return VerifyResult(-EINVAL);
  header = *image;
  if (header.size + sizeof(header) != size || header.size > Code.size())
    return VerifyResult(-EINVAL);
  Buffer inb(image->code, header.size);
  Buffer hsh(crypto::SHA256(inb));
  Buffer N(image->key, header.key_size);
  // BUG: header.key_size can be longer than kSignKeyLength
  // -> key for decryption is longer than the constexpr one
  if (memcmp(N.ptr(), kSignKey, kSignKeyLength))
    return VerifyResult(-EKEYREJECTED);
  Buffer sign(image->signature, header.key_size);
  uint32_t e = 0x10001;
  Buffer E((const uint8_t *)&e, sizeof(e));
  Buffer res(crypto::RSA_encrypt(N, E, sign));
  if (!hsh.ValueEq(res))
    return VerifyResult(-EBADMSG);
  memcpy(Code.at(0), inb.ptr(), inb.size());
  VerifyResult(0);
}

void RunMain() {
  constexpr int kEventFdFromHost = 5;
  constexpr int kEventFdToHost = 6;
  Event from(kEventFdFromHost), to(kEventFdToHost);
  flag_firmware = open("flag_firmware", O_RDONLY);
  flag_sandbox = open("flag_sandbox", O_RDONLY);
  CHECK(flag_firmware >= 0);
  CHECK(flag_sandbox >= 0);
  install_seccomp();

  from.WaitAndClear();
  VerifyFirmware();
  to.Trigger(0x1337);
  while (1) {
    from.WaitAndClear();
    if (Sandboxing())
      to.Trigger();
  }
}

} // namespace

int main() {
  RunMain();
  return 0;
}
