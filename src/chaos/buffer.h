/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _BUFFER_H
#define _BUFFER_H

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "check.h"
#include "inferior.h"

class Buffer {
 public:
  Buffer(const uint8_t *from, uint32_t& size) : Buffer(size) {
    if (Allocate())
      memcpy(ptr_, from, size_);
  }

  Buffer(uint32_t size) : ptr_(nullptr), size_(size) {
    debug("size 0x%x\n", size_);
  }
  ~Buffer() {
    debug("size 0x%x\n", size_);
    if (ptr_)
      delete ptr_;
  }

  bool FromUser(Inferior &inferior, uint32_t uptr) {
    if (!ptr_ && !Allocate())
      return false;
    return inferior.Read(ptr_, uptr, size_);
  }

  bool ToUser(Inferior &inferior, uint32_t uptr) {
    CHECK(ptr_);
    return inferior.Write(ptr_, uptr, size_);
  }

  bool Allocate() {
    if (ptr_)
      return false;
    if (size_ == 0 || size_ > 0x100000)
      return false;
    // NOTE: can be a bug if size_ is not word-aligned
    // PEEKUSER can overflow
    ptr_ = new uint8_t[size_ + 8];
    if (!ptr_)
      return false;
    return true;
  }

  inline uint8_t *ptr() const { return ptr_; }
  inline uint32_t size() const { return size_; }

 private:
  uint8_t *ptr_;
  uint32_t size_;
};

#endif // _BUFFER_H
