/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "crypto.h"

#include <gmp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

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

Buffer SHA256(const Buffer &inb) {
  Buffer out(SHA256_DIGEST_LENGTH);
  CHECK(out.Allocate());
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  SHA256_Update(&ctx, inb.ptr(), inb.size());
  SHA256_Final(out.ptr(), &ctx);
  return out;
}

Buffer RSA_encrypt(const Buffer &N, const Buffer &E, const Buffer &inb) {
    Buffer out(N.size());
    size_t size;
    CHECK(out.Allocate());
    mpz_t in, result, e, n;
    mpz_init(in);
    mpz_init(result);
    mpz_init(e);
    mpz_init(n);
    mpz_import(in, inb.size(), -1, 1, 0, 0, inb.ptr());
    mpz_import(e, E.size(), -1, 1, 0, 0, E.ptr());
    mpz_import(n, N.size(), -1, 1, 0, 0, N.ptr());
    mpz_powm(result, in, e, n);
    mpz_export(out.ptr(), &size, -1, 1, 0, 0, result);
    CHECK(size <= out.size());
    mpz_clear(n);
    mpz_clear(e);
    mpz_clear(result);
    mpz_clear(in);
    return out;
}

}
