/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _THREEFISH_H
#define _THREEFISH_H

#include <cstddef>
#include <cstdint>

namespace threefish {

constexpr size_t kBlockSize = 32;
constexpr size_t kKeyLength = 32;

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb);

void decrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb);

}
#endif // _THREEFISH_H
