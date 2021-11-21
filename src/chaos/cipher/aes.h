/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _AES_H
#define _AES_H

#include <cstdint>

namespace aes {

void encrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

void decrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

}

#endif // _AES_H
