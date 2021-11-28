/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include "twofish.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace {

constexpr uint32_t G_M = 0x169;

constexpr uint8_t tab_5b[4] = { 0, G_M >> 2, G_M >> 1, (G_M >> 1) ^ (G_M >> 2) };
constexpr uint8_t tab_ef[4] = { 0, (G_M >> 1) ^ (G_M >> 2), G_M >> 1, G_M >> 2 };

#define ffm_01(x) (x)
#define ffm_5b(x) ((x) ^ ((x) >> 2) ^ tab_5b[(x) & 3])
#define ffm_ef(x) ((x) ^ ((x) >> 1) ^ ((x) >> 2) ^ tab_ef[(x) & 3])

constexpr uint8_t ror4[16] = { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15 };
constexpr uint8_t ashx[16] = { 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12, 5, 14, 7 };

constexpr uint8_t qt0[2][16] = {
    { 8, 1, 7, 13, 6, 15, 3, 2, 0, 11, 5, 9, 14, 12, 10, 4 },
    { 2, 8, 11, 13, 15, 7, 6, 14, 3, 1, 9, 4, 0, 10, 12, 5 }
};

constexpr uint8_t qt1[2][16] = {
    { 14, 12, 11, 8, 1, 2, 3, 5, 15, 4, 10, 6, 7, 0, 9, 13 },
    { 1, 14, 2, 11, 4, 12, 3, 7, 6, 13, 10, 5, 15, 9, 0, 8 }
};

constexpr uint8_t qt2[2][16] = {
    { 11, 10, 5, 14, 6, 13, 9, 0, 12, 8, 15, 3, 2, 4, 7, 1 },
    { 4, 12, 7, 5, 1, 6, 9, 10, 0, 14, 13, 8, 2, 11, 3, 15 }
};

constexpr uint8_t qt3[2][16] = {
    { 13, 7, 15, 4, 1, 2, 6, 14, 9, 11, 3, 0, 8, 5, 12, 10 },
    { 11, 9, 5, 1, 12, 3, 13, 14, 6, 4, 7, 15, 2, 0, 8, 10 }
};

uint8_t q_tab[2][256];

uint32_t s_key[2];
uint32_t l_key[40];
uint32_t m_tab[4][256];
uint32_t mk_tab[4][256];

uint8_t qp(const uint32_t n, uint8_t x) {
  uint8_t a0, a1, a2, a3, a4, b0, b1, b2, b3, b4;
  a0 = x >> 4;
  b0 = x & 15;
  a1 = a0 ^ b0;
  b1 = ror4[b0] ^ ashx[a0];
  a2 = qt0[n][a1];
  b2 = qt1[n][b1];
  a3 = a2 ^ b2;
  b3 = ror4[b2] ^ ashx[a2];
  a4 = qt2[n][a3];
  b4 = qt3[n][b3];
  return (b4 << 4) | a4;
}

void gen_qtab() {
  for (int i = 0; i < 256; ++i) {
    q_tab[0][i] = qp(0, i);
    q_tab[1][i] = qp(1, i);
  }
}

#define q(n,x)  q_tab[n][x]

void gen_mtab() {
  uint32_t f01, f5b, fef;
  for(int i = 0; i < 256; ++i) {
    f01 = q(1,i); f5b = ffm_5b(f01); fef = ffm_ef(f01);
    m_tab[0][i] = f01 + (f5b << 8) + (fef << 16) + (fef << 24);
    m_tab[2][i] = f5b + (fef << 8) + (f01 << 16) + (fef << 24);

    f01 = q(0,i); f5b = ffm_5b(f01); fef = ffm_ef(f01);
    m_tab[1][i] = fef + (fef << 8) + (f5b << 16) + (f01 << 24);
    m_tab[3][i] = f5b + (f01 << 8) + (fef << 16) + (f5b << 24);
  }
}

#define BYTE(x, n) ((x>>(8 * n)) & 0xff)

uint32_t h(uint32_t x, const uint32_t key[]) {
  uint32_t b0, b1, b2, b3;
  b0 = BYTE(x, 0); b1 = BYTE(x, 1); b2 = BYTE(x, 2); b3 = BYTE(x, 3);

  b0 = q(0, q(0, b0) ^ BYTE(key[1], 0)) ^ BYTE(key[0], 0);
  b1 = q(0, q(1, b1) ^ BYTE(key[1], 1)) ^ BYTE(key[0], 1);
  b2 = q(1, q(0, b2) ^ BYTE(key[1], 2)) ^ BYTE(key[0], 2);
  b3 = q(1, q(1, b3) ^ BYTE(key[1], 3)) ^ BYTE(key[0], 3);

  return m_tab[0][b0] ^ m_tab[1][b1] ^ m_tab[2][b2] ^ m_tab[3][b3];
}

#define q20(x) (q(0, q(0, x) ^ BYTE(s_key[1], 0)) ^ BYTE(s_key[0], 0))
#define q21(x) (q(0, q(1, x) ^ BYTE(s_key[1], 1)) ^ BYTE(s_key[0], 1))
#define q22(x) (q(1, q(0, x) ^ BYTE(s_key[1], 2)) ^ BYTE(s_key[0], 2))
#define q23(x) (q(1, q(1, x) ^ BYTE(s_key[1], 3)) ^ BYTE(s_key[0], 3))

void gen_mk_tab() {
  for (int i = 0; i < 256; ++i) {
    mk_tab[0][i] = m_tab[0][q20((uint8_t)i)];
    mk_tab[1][i] = m_tab[1][q21((uint8_t)i)];
    mk_tab[2][i] = m_tab[2][q22((uint8_t)i)];
    mk_tab[3][i] = m_tab[3][q23((uint8_t)i)];
  }
}

#define rotr(x,n) (((x) >> ((int)(n))) | ((x) << (32 - (int)(n))))
#define rotl(x,n) (((x) << ((int)(n))) | ((x) >> (32 - (int)(n))))

constexpr uint32_t G_MOD = 0x14d;

uint32_t mds_rem(uint32_t p0, uint32_t p1) {
  for (int i = 0; i < 8; ++i) {
    uint32_t t = p1 >> 24;
    p1 = (p1 << 8) | (p0 >> 24);
    p0 <<= 8;
    uint32_t u = (t << 1);
    if(t & 0x80)
      u ^= G_MOD;
    p1 ^= t ^ (u << 16);
    u ^= (t >> 1);
    if(t & 0x01)
      u ^= G_MOD >> 1;
    p1 ^= (u << 24) | (u << 8);
  }
  return p1;
}

void setkey(uint32_t *in_key) {
  uint32_t me_key[4], mo_key[4];
  gen_qtab();
  gen_mtab();
  for (int i = 0; i < 2; ++i) {
    me_key[i] = in_key[2 * i];
    mo_key[i] = in_key[2 * i + 1];
    s_key[1 - i] = mds_rem(me_key[i], mo_key[i]);
  }
  for (int i = 0; i < 40; i += 2) {
    uint32_t a = 0x01010101 * i;
    uint32_t b = a + 0x01010101;
    a = h(a, me_key);
    b = rotl(h(b, mo_key), 8);
    l_key[i] = a + b;
    l_key[i + 1] = rotl(a + 2 * b, 9);
  }
  gen_mk_tab();
}

#define g0(x) ( mk_tab[0][BYTE(x,0)] ^ mk_tab[1][BYTE(x,1)] \
                      ^ mk_tab[2][BYTE(x,2)] ^ mk_tab[3][BYTE(x,3)] )
#define g1(x) ( mk_tab[0][BYTE(x,3)] ^ mk_tab[1][BYTE(x,0)] \
                      ^ mk_tab[2][BYTE(x,1)] ^ mk_tab[3][BYTE(x,2)] )

void enc(uint32_t *data) {
  uint32_t t0, t1, blk[4];
  for (int i = 0; i < 4; i++)
    blk[i] = data[i] ^ l_key[i];

  for (int i = 0; i < 8; i++) {
    t1 = g1(blk[1]); t0 = g0(blk[0]);
    blk[2] = rotr(blk[2] ^ (t0 + t1 + l_key[4 * (i) + 8]), 1);
    blk[3] = rotl(blk[3], 1) ^ (t0 + 2 * t1 + l_key[4 * (i) + 9]);
    t1 = g1(blk[3]); t0 = g0(blk[2]);
    blk[0] = rotr(blk[0] ^ (t0 + t1 + l_key[4 * (i) + 10]), 1);
    blk[1] = rotl(blk[1], 1) ^ (t0 + 2 * t1 + l_key[4 * (i) + 11]);
  }
  for (int i = 0; i < 4; i++)
    data[i] = blk[(i + 2) % 4] ^ l_key[i + 4];
}

void dec(uint32_t *data) {
  uint32_t t0, t1, blk[4];
  for (int i = 0; i < 4; i++)
    blk[i] = data[i] ^ l_key[i + 4];
  for (int i = 7; i >= 0; i--) {
    t1 = g1(blk[1]); t0 = g0(blk[0]);
    blk[2] = rotl(blk[2], 1) ^ (t0 + t1 + l_key[4 * (i) + 10]);
    blk[3] = rotr(blk[3] ^ (t0 + 2 * t1 + l_key[4 * (i) + 11]), 1);
    t1 = g1(blk[3]); t0 = g0(blk[2]);
    blk[0] = rotl(blk[0], 1) ^ (t0 + t1 + l_key[4 * (i) +  8]);
    blk[1] = rotr(blk[1] ^ (t0 + 2 * t1 + l_key[4 * (i) +  9]), 1);
  }
  for (int i = 0; i < 4; i++)
    data[i] = blk[(i + 2) % 4] ^ l_key[i];
}

} // namespace

namespace twofish {

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb) {
  memcpy(outb, inb, kBlockSize);
  setkey((uint32_t *)key);
  enc((uint32_t *)outb);
}

void decrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb) {
  memcpy(outb, inb, kBlockSize);
  setkey((uint32_t *)key);
  dec((uint32_t *)outb);
}

}
