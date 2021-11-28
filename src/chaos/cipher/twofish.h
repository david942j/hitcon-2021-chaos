/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _TWOFISH_H
#define _TWOFISH_H

#include <cstddef>
#include <cstdint>

namespace twofish {

constexpr size_t kBlockSize = 16;
constexpr size_t kKeyLength = 16;

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb);

void decrypt(uint8_t *key, size_t klen, uint8_t *inb, uint8_t *outb);

}
#endif // _TWOFISH_H
