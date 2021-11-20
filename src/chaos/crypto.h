/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "buffer.h"

namespace crypto {

Buffer MD5(const Buffer &inb);

Buffer SHA256(const Buffer &inb);

Buffer RSA_encrypt(const Buffer &N, const Buffer &E, const Buffer &inb);

Buffer RSA_decrypt(const Buffer &N, const Buffer &D, const Buffer &inb);

Buffer AES_encrypt(const Buffer &key, const Buffer &inb);

Buffer AES_decrypt(const Buffer &key, const Buffer &inb);

Buffer RC4_encrypt(const Buffer &key, const Buffer &inb);

Buffer RC4_decrypt(const Buffer &key, const Buffer &inb);

}

#endif // _CRYPTO_H
