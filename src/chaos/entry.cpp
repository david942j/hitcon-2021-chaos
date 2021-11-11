/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <cstdio>
#include <sys/mman.h>
#include <cassert>
#include <cstdint>
#include <unistd.h>

#define CSR_FD 3
#define CSR_BASE 0x10000
#define DRAM_FD 4
#define DRAM_BASE 0x10000000

class MemoryRegion {
 public:
  MemoryRegion(int fd, uint64_t base) : fd_(fd), base_((void*)base) {
    lseek(fd, 0, SEEK_SET);
    size_ = lseek(fd, 0, SEEK_END);
    auto ptr = mmap(base_, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(ptr == base_);
  }
  ~MemoryRegion() {
    munmap(base_, size_);
  }

 private:
  int fd_;
  void *base_;
  uint64_t size_;
};

static MemoryRegion CSR(CSR_FD, CSR_BASE);
static MemoryRegion DRAM(DRAM_FD, DRAM_BASE);

int main() {
  alarm(60);
  while(1);
  return 0;
}
