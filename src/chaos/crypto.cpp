/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "crypto.h"

#include <openssl/md5.h>
#include <openssl/sha.h>

#include "check.h"
#include "buffer.h"
#include "cipher/aes.h"
#include "cipher/rc4.h"
#include "cipher/rsa.h"

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
    CHECK(out.Allocate());
    rsa::encrypt(N.ptr(), N.size(), E.ptr(), E.size(), inb.ptr(), inb.size(), out.ptr());
    return out;
}

Buffer AES_encrypt(const Buffer &key, const Buffer &inb) {
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  aes::encrypt(key.ptr(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer AES_decrypt(const Buffer &key, const Buffer &inb) {
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  aes::decrypt(key.ptr(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer RC4_encrypt(const Buffer &key, const Buffer &inb) {
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  rc4::encrypt(key.ptr(), key.size(), inb.ptr(), inb.size(), outb.ptr());
  return outb;
}

Buffer RC4_decrypt(const Buffer &key, const Buffer &inb) {
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  rc4::decrypt(key.ptr(), key.size(), inb.ptr(), inb.size(), outb.ptr());
  return outb;
}

}
