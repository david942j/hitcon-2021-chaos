/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _RC4_H
#define _RC4_H

#include <cstddef>
#include <cstdint>

namespace rc4 {

void encrypt(uint8_t *key, size_t klen, uint8_t *inb, size_t len, uint8_t *outb);

// RC4 decryption is identical to encryption.
const auto decrypt = encrypt;

}

#endif // _RC4_H
