/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include "threefish.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace {

uint64_t m_rkey[5];

#define rotr(x,n) (((x) >> ((int)(n))) | ((x) << (64 - (int)(n))))
#define rotl(x,n) (((x) << ((int)(n))) | ((x) >> (64 - (int)(n))))

template <unsigned int C0, unsigned int C1>
inline void G256(uint64_t& G0, uint64_t& G1, uint64_t& G2, uint64_t& G3) {
  G0 += G1;
  G1 = rotl(G1, C0) ^ G0;
  G2 += G3;
  G3 = rotl(G3, C1) ^ G2;
}

template <unsigned int C0, unsigned int C1>
inline void IG256(uint64_t& G0, uint64_t& G1, uint64_t& G2, uint64_t& G3) {
  G3 = rotr(G3 ^ G2, C1);
  G2 -= G3;
  G1 = rotr(G1 ^ G0, C0);
  G0 -= G1;
}

#define KS256(r) \
    G0 += m_rkey[(r + 1) % 5]; \
    G1 += m_rkey[(r + 2) % 5]; \
    G2 += m_rkey[(r + 3) % 5]; \
    G3 += m_rkey[(r + 4) % 5] + r + 1;

#define IKS256(r) \
    G0 -= m_rkey[(r + 1) % 5]; \
    G1 -= m_rkey[(r + 2) % 5]; \
    G2 -= m_rkey[(r + 3) % 5]; \
    G3 -= m_rkey[(r + 4) % 5] + r + 1;

void setkey(uint64_t *in_key) {
  for (int i = 0; i < 4 ; i++)
    m_rkey[i] = in_key[i];
  m_rkey[4] = 0x1BD11BDAA9FC1A22 ^ m_rkey[0] ^ m_rkey[1] ^ m_rkey[2] ^ m_rkey[3];
}

void enc(uint64_t *data) {
  uint64_t G0 = data[0] + m_rkey[0];
  uint64_t G1 = data[1] + m_rkey[1];
  uint64_t G2 = data[2] + m_rkey[2];
  uint64_t G3 = data[3] + m_rkey[3];
  for (int i = 0; i < 9; i++) {
    G256<14, 16>(G0, G1, G2, G3);
    G256<52, 57>(G0, G3, G2, G1);
    G256<23, 40>(G0, G1, G2, G3);
    G256< 5, 37>(G0, G3, G2, G1);
    KS256(2 * i);
    G256<25, 33>(G0, G1, G2, G3);
    G256<46, 12>(G0, G3, G2, G1);
    G256<58, 22>(G0, G1, G2, G3);
    G256<32, 32>(G0, G3, G2, G1);
    KS256(2 * i + 1);
  }
  data[0] = G0;
  data[1] = G1;
  data[2] = G2;
  data[3] = G3;
}

void dec(uint64_t *data) {
  uint64_t G0 = data[0] - m_rkey[3];
  uint64_t G1 = data[1] - m_rkey[4];
  uint64_t G2 = data[2] - m_rkey[0];
  uint64_t G3 = data[3] - m_rkey[1] - 18;
  for (int i = 8; i >= 0; i--) {
    IG256<32, 32>(G0, G3, G2, G1);
    IG256<58, 22>(G0, G1, G2, G3);
    IG256<46, 12>(G0, G3, G2, G1);
    IG256<25, 33>(G0, G1, G2, G3);
    IKS256(2 * i);
    IG256< 5, 37>(G0, G3, G2, G1);
    IG256<23, 40>(G0, G1, G2, G3);
    IG256<52, 57>(G0, G3, G2, G1);
    IG256<14, 16>(G0, G1, G2, G3);
    IKS256(2 * i - 1);
  }
  data[0] = G0;
  data[1] = G1;
  data[2] = G2;
  data[3] = G3;
}

} // namespace



namespace threefish {

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb) {
  memcpy(outb, inb, kBlockSize);
  setkey((uint64_t *)key);
  enc((uint64_t *)outb);
}

void decrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb) {
  memcpy(outb, inb, kBlockSize);
  setkey((uint64_t *)key);
  dec((uint64_t *)outb);
}

}
