/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "crypto.h"

#include <openssl/md5.h>

#include "buffer.h"

namespace crypto {

Buffer MD5(const Buffer &inb) {
  Buffer out(MD5_DIGEST_LENGTH);
  CHECK(out.Allocate());
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, inb.ptr(), inb.size());
  MD5_Final(out.ptr(), &ctx);
  return out;
}

}
