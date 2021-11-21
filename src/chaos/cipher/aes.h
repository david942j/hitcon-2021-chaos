/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _AES_H
#define _AES_H

#include <cstddef>
#include <cstdint>

namespace aes {

constexpr size_t kBlockSize = 16;
constexpr size_t kKeyLength = 16;

void encrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

void decrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

}

#endif // _AES_H
