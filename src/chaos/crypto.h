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

}

#endif // _CRYPTO_H
