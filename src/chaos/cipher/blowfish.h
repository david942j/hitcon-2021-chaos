/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _BLOWFISH_H
#define _BLOWFISH_H

#include <cstddef>
#include <cstdint>

namespace blowfish {

constexpr size_t kBlockSize = 8;
constexpr size_t kMaxKeyLength = 56;

void encrypt(uint32_t *key, size_t klen, uint32_t *inb, uint32_t *outb);

void decrypt(uint32_t *key, size_t klen, uint32_t *inb, uint32_t *outb);

}
#endif // _BLOWFISH_H
