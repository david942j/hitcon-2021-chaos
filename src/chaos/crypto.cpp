/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "crypto.h"

#include <openssl/md5.h>
#include <openssl/sha.h>

#include "buffer.h"
#include "check.h"
#include "cipher/aes.h"
#include "cipher/blowfish.h"
#include "cipher/rc4.h"
#include "cipher/rsa.h"
#include "cipher/threefish.h"
#include "cipher/twofish.h"

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
  CHECK(inb.size() <= N.size());
  rsa::encrypt(N.ptr(), N.size(), E.ptr(), E.size(), inb.ptr(), inb.size(), out.ptr());
  return out;
}

Buffer RSA_decrypt(const Buffer &N, const Buffer &D, const Buffer &inb) {
  Buffer out(N.size());
  CHECK(out.Allocate());
  CHECK(inb.size() <= N.size());
  rsa::decrypt(N.ptr(), N.size(), D.ptr(), D.size(), inb.ptr(), inb.size(), out.ptr());
  return out;
}

Buffer AES_encrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == aes::kKeyLength);
  CHECK(inb.size() <= aes::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  aes::encrypt(key.ptr(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer AES_decrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == aes::kKeyLength);
  CHECK(inb.size() <= aes::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  aes::decrypt(key.ptr(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer RC4_encrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() > 0);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  rc4::encrypt(key.ptr(), key.size(), inb.ptr(), inb.size(), outb.ptr());
  return outb;
}

Buffer RC4_decrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() > 0);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  rc4::decrypt(key.ptr(), key.size(), inb.ptr(), inb.size(), outb.ptr());
  return outb;
}

Buffer BLOWFISH_encrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() % sizeof(uint32_t) == 0);
  CHECK(0 < key.size() && key.size() <= blowfish::kMaxKeyLength);
  CHECK(inb.size() % sizeof(uint32_t) == 0);
  CHECK(inb.size() <= blowfish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  blowfish::encrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer BLOWFISH_decrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() % sizeof(uint32_t) == 0);
  CHECK(0 < key.size() && key.size() <= blowfish::kMaxKeyLength);
  CHECK(inb.size() % sizeof(uint32_t) == 0);
  CHECK(inb.size() <= blowfish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  blowfish::decrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer TWOFISH_encrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == twofish::kKeyLength);
  CHECK(inb.size() % sizeof(uint32_t) == 0);
  CHECK(inb.size() <= twofish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  twofish::encrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer TWOFISH_decrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == twofish::kKeyLength);
  CHECK(inb.size() % sizeof(uint32_t) == 0);
  CHECK(inb.size() <= twofish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  twofish::decrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer THREEFISH_encrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == threefish::kKeyLength);
  CHECK(inb.size() % sizeof(uint64_t) == 0);
  CHECK(inb.size() <= threefish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  threefish::encrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

Buffer THREEFISH_decrypt(const Buffer &key, const Buffer &inb) {
  CHECK(key.size() == threefish::kKeyLength);
  CHECK(inb.size() % sizeof(uint64_t) == 0);
  CHECK(inb.size() <= threefish::kBlockSize);
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  threefish::decrypt(key.ptr(), key.size(), inb.ptr(), outb.ptr());
  return outb;
}

}
