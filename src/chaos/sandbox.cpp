/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <sys/eventfd.h>
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

enum chaos_request_algo {
  /* copy input to output, for testing purpose */
  CHAOS_ALGO_ECHO,
  CHAOS_ALGO_MD5,
};

long HandleCryptoCall(Inferior &inferior, const uint64_t *args) {
  switch (args[0]) {
  case CHAOS_ALGO_MD5: {
    uint32_t in = args[1];
    uint32_t in_size = args[2];
    uint32_t out = args[3];
    Buffer inb(in_size);
    if (!inb.FromUser(inferior, in))
      return -EFAULT;
    Buffer outb(crypto::MD5(inb));
    if (!outb.ToUser(inferior, out))
      return -EFAULT;
    return outb.size();
  }
  default:
    return -ENOSYS;
  }
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

#define SIGN_LENGTH (1024 / 8)
struct ImageHeader {
  uint32_t size; // image size without header
  uint8_t signature[SIGN_LENGTH];
  uint8_t code[];
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
  // TODO: verify the signature
  memcpy(Code.at(0), inb.ptr(), inb.size());
  VerifyResult(0);
}

void RunMain() {
  constexpr int kEventFdFromHost = 5;
  constexpr int kEventFdToHost = 6;
  Event from(kEventFdFromHost), to(kEventFdToHost);

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
