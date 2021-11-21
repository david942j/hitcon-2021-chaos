/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#ifndef _AES_H
#define _AES_H

#include <cstdint>

#define AES_BLOCK_SIZE 16
#define AES_KEY_LENGTH 16

namespace aes {

void encrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

void decrypt(uint8_t *key, uint8_t *inb, uint8_t *outb);

}

#endif // _AES_H
