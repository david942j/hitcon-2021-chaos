/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include "aes.h"

#include <cstdint>
#include <cstring>

namespace {

uint8_t log_table[256] = {0,0,25,1,50,2,26,198,75,199,27,104,51,238,223,3,100,4,224,14,52,141,129,239,76,113,8,200,248,105,28,193,125,194,29,181,249,185,39,106,77,228,166,114,154,201,9,120,101,47,138,5,33,15,225,36,18,240,130,69,53,147,218,142,150,143,219,189,54,208,206,148,19,92,210,241,64,70,131,56,102,221,253,48,191,6,139,98,179,37,226,152,34,136,145,16,126,110,72,195,163,182,30,66,58,107,40,84,250,133,61,186,43,121,10,21,155,159,94,202,78,212,172,229,243,115,167,87,175,88,168,80,244,234,214,116,79,174,233,213,231,230,173,232,44,215,117,122,235,22,11,245,89,203,95,176,156,169,81,160,127,12,246,111,23,196,73,236,216,67,31,45,164,118,123,183,204,187,62,90,251,96,177,134,59,82,161,108,170,85,41,157,151,178,135,144,97,190,220,252,188,149,207,205,55,63,91,209,83,57,132,60,65,162,109,71,20,42,158,93,86,242,211,171,68,17,146,217,35,32,46,137,180,124,184,38,119,153,227,165,103,74,237,222,197,49,254,24,13,99,140,128,192,247,112,7};
uint8_t exp_table[256] = {1,3,5,15,17,51,85,255,26,46,114,150,161,248,19,53,95,225,56,72,216,115,149,164,247,2,6,10,30,34,102,170,229,52,92,228,55,89,235,38,106,190,217,112,144,171,230,49,83,245,4,12,20,60,68,204,79,209,104,184,211,110,178,205,76,212,103,169,224,59,77,215,98,166,241,8,24,40,120,136,131,158,185,208,107,189,220,127,129,152,179,206,73,219,118,154,181,196,87,249,16,48,80,240,11,29,39,105,187,214,97,163,254,25,43,125,135,146,173,236,47,113,147,174,233,32,96,160,251,22,58,78,210,109,183,194,93,231,50,86,250,21,63,65,195,94,226,61,71,201,64,192,91,237,44,116,156,191,218,117,159,186,213,100,172,239,42,126,130,157,188,223,122,142,137,128,155,182,193,88,232,35,101,175,234,37,111,177,200,67,197,84,252,31,33,99,165,244,7,9,27,45,119,153,176,203,70,202,69,207,74,222,121,139,134,145,168,227,62,66,198,81,243,14,18,54,90,238,41,123,141,140,143,138,133,148,167,242,13,23,57,75,221,124,132,151,162,253,28,36,108,180,199,82,246,1};
uint8_t sbox[256] = {99,124,119,123,242,107,111,197,48,1,103,43,254,215,171,118,202,130,201,125,250,89,71,240,173,212,162,175,156,164,114,192,183,253,147,38,54,63,247,204,52,165,229,241,113,216,49,21,4,199,35,195,24,150,5,154,7,18,128,226,235,39,178,117,9,131,44,26,27,110,90,160,82,59,214,179,41,227,47,132,83,209,0,237,32,252,177,91,106,203,190,57,74,76,88,207,208,239,170,251,67,77,51,133,69,249,2,127,80,60,159,168,81,163,64,143,146,157,56,245,188,182,218,33,16,255,243,210,205,12,19,236,95,151,68,23,196,167,126,61,100,93,25,115,96,129,79,220,34,42,144,136,70,238,184,20,222,94,11,219,224,50,58,10,73,6,36,92,194,211,172,98,145,149,228,121,231,200,55,109,141,213,78,169,108,86,244,234,101,122,174,8,186,120,37,46,28,166,180,198,232,221,116,31,75,189,139,138,112,62,181,102,72,3,246,14,97,53,87,185,134,193,29,158,225,248,152,17,105,217,142,148,155,30,135,233,206,85,40,223,140,161,137,13,191,230,66,104,65,153,45,15,176,84,187,22};
uint8_t invsbox[256] = {0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d};
uint8_t rc[32] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36,0x6C,0xD8,0xAB,0x4D,0x9A,0x2F,0x5E,0xBC,0x63,0xc6,0x97,0x35,0x6A,0xD4,0xB3,0x7D,0xFA,0xEF,0xC5};

constexpr int kRound = 10;
using aes::kBlockSize;

void setRoundKey(uint8_t *key, uint8_t roundkey[kRound + 1][kBlockSize]){
  for (size_t i = 0; i < kBlockSize; i++) {
    roundkey[0][i] = key[i];
  }
  for (int i = 1; i <= kRound; i++) {
    uint8_t tmp[4];
    for (int j = 0; j < 3; j++) {
      tmp[j] = roundkey[i - 1][13 + j];
    }
    tmp[3] = roundkey[i - 1][12];
    for (int j = 0; j < 4; j++) {
      tmp[j] = sbox[tmp[j]];
    }
    tmp[0] ^= rc[i];
    for (int j = 0; j < 4; j++) {
      roundkey[i][j] = roundkey[i - 1][j] ^ tmp[j];
    }
    for (size_t j = 4; j < kBlockSize; j++) {
      roundkey[i][j] = roundkey[i - 1][j] ^ roundkey[i][j - 4];
    }
  }
}

uint8_t multi(uint8_t a, uint8_t b) {
  if (a == 0 || b == 0) return 0;
  return exp_table[(log_table[a] + log_table[b]) % 255];
}

} // namespace

namespace aes {

void encrypt(uint8_t *key, uint8_t *inb, uint8_t *outb) {
  uint8_t roundkey[kRound + 1][kBlockSize];
  setRoundKey(key, roundkey);
  memcpy(outb, inb, kBlockSize);
  for (size_t i = 0; i < kBlockSize; i++) {
    outb[i] ^= roundkey[0][i];
  }
  for (int r = 1; r <= kRound; r++) {
    for (size_t i = 0; i < kBlockSize; i++)
      outb[i] = sbox[outb[i]];
    uint8_t tmp;
    tmp = outb[1]; outb[1] = outb[5]; outb[5] = outb[9]; outb[9] = outb[13]; outb[13] = tmp;
    tmp = outb[2]; outb[2] = outb[10]; outb[10] = tmp;
    tmp = outb[6]; outb[6] = outb[14]; outb[14] = tmp;
    tmp = outb[3]; outb[3] = outb[15]; outb[15] = outb[11]; outb[11] = outb[7]; outb[7] = tmp;
    if (r != kRound) {
      for (int i = 0; i < 4; i++) {
        uint8_t poly[4];
        for (int j = 0; j < 4; j++) {
          poly[j] = outb[4 * i + j];
        }
        outb[4 * i]     = multi(2, poly[0]) ^ multi(3, poly[1]) ^ poly[2] ^ poly[3];
        outb[4 * i + 1] = multi(2, poly[1]) ^ multi(3, poly[2]) ^ poly[3] ^ poly[0];
        outb[4 * i + 2] = multi(2, poly[2]) ^ multi(3, poly[3]) ^ poly[0] ^ poly[1];
        outb[4 * i + 3] = multi(2, poly[3]) ^ multi(3, poly[0]) ^ poly[1] ^ poly[2];
      }
    }
    for (size_t i = 0; i < kBlockSize; i++) {
      outb[i] ^= roundkey[r][i];
    }
  }
}

void decrypt(uint8_t *key, uint8_t *inb, uint8_t *outb) {
  uint8_t roundkey[kRound + 1][kBlockSize];
  setRoundKey(key, roundkey);
  memcpy(outb, inb, kBlockSize);
  for (int r = kRound; r >= 1; r--) {
    for (size_t i = 0; i < kBlockSize; i++) {
      outb[i] ^= roundkey[r][i];
    }
    if (r != kRound){
      uint8_t poly[4];
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
          poly[j] = outb[4 * i + j];
        }
        outb[4 * i]     = multi(0xe, poly[0]) ^ multi(0xb, poly[1]) ^ multi(0xd, poly[2]) ^ multi(0x9, poly[3]);
        outb[4 * i + 1] = multi(0x9, poly[0]) ^ multi(0xe, poly[1]) ^ multi(0xb, poly[2]) ^ multi(0xd, poly[3]);
        outb[4 * i + 2] = multi(0xd, poly[0]) ^ multi(0x9, poly[1]) ^ multi(0xe, poly[2]) ^ multi(0xb, poly[3]);
        outb[4 * i + 3] = multi(0xb, poly[0]) ^ multi(0xd, poly[1]) ^ multi(0x9, poly[2]) ^ multi(0xe, poly[3]);
      }
    }
    uint8_t tmp;
    tmp = outb[1]; outb[1] = outb[13]; outb[13] = outb[9]; outb[9] = outb[5]; outb[5] = tmp;
    tmp = outb[2]; outb[2] = outb[10]; outb[10] = tmp;
    tmp = outb[6]; outb[6] = outb[14]; outb[14] = tmp;
    tmp = outb[3]; outb[3] = outb[7]; outb[7] = outb[11]; outb[11] = outb[15]; outb[15] = tmp;
    for (size_t i = 0; i < kBlockSize; i++) {
      outb[i] = invsbox[outb[i]];
    }
  }
  for (size_t i = 0; i < kBlockSize; i++) {
    outb[i] ^= roundkey[0][i];
  }
}

} // namespace aes
