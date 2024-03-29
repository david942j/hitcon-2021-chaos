/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _RSA_H
#define _RSA_H

#include <cstddef>
#include <cstdint>

namespace rsa {

void encrypt(uint8_t *N, size_t nlen, uint8_t *E, size_t elen, uint8_t *inb, size_t len, uint8_t *outb);

// RSA decryption is identical to encryption.
const auto decrypt = encrypt;

}

#endif // _RSA_H
