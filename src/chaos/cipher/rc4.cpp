/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include <cstdint>
#include <cstring>

namespace rc4 {

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, size_t len, uint8_t *outb) {
  int k, j = 0;
  uint8_t sbox[256], tmp;
  for (int i=0;i<256;i++)
    sbox[i] = i;
  for (int i=0;i<256;i++) {
    j = (j + sbox[i] + key[i%klen]) % 256;
    tmp = sbox[i]; sbox[i] = sbox[j]; sbox[j] = tmp;
  }
  k = j = 0;
  for (int i=0;i<len;i++) {
    k = (k + 1 ) % 256;
    j = (j + sbox[k]) % 256;
    tmp = sbox[k]; sbox[k] = sbox[j]; sbox[j] = tmp;
    outb[i] = inb[i] ^ sbox[(sbox[k] + sbox[j])%256];
  }
}

void decrypt(uint8_t *key, size_t klen, uint8_t *inb, size_t len, uint8_t *outb) {
    encrypt(key, klen, inb, len, outb);
}

}
