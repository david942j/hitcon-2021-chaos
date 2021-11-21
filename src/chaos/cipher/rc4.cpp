/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include "rc4.h"

#include <cstdint>
#include <cstddef>
#include <utility>

namespace rc4 {

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, size_t len, uint8_t *outb) {
  constexpr int kBoxSize = 256;
  uint8_t sbox[kBoxSize];
  for (int i = 0; i < kBoxSize; i++) {
    sbox[i] = i;
  }
  int k = 0, j = 0;
  for (int i = 0; i < kBoxSize; i++) {
    j = (j + sbox[i] + key[i % klen]) % kBoxSize;
    std::swap(sbox[i], sbox[j]);
  }
  j = 0;
  for (size_t i = 0; i < len; i++) {
    k = (k + 1) % kBoxSize;
    j = (j + sbox[k]) % kBoxSize;
    std::swap(sbox[j], sbox[k]);
    outb[i] = inb[i] ^ sbox[(sbox[k] + sbox[j]) % kBoxSize];
  }
}

}
