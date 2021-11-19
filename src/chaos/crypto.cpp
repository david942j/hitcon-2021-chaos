/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "crypto.h"

#include <gmp.h>
#include <openssl/evp.h>
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

Buffer AES_encrypt(const Buffer &key, const Buffer &iv, const Buffer &inb) {
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  Buffer outb(4 + inb.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
  CHECK(outb.Allocate());
  int size1, size2;
  EVP_CIPHER_CTX_init(ctx);
  EVP_EncryptInit(ctx, EVP_aes_256_cbc(), key.ptr(), iv.ptr());
  EVP_EncryptUpdate(ctx, outb.ptr() + 4, &size1, inb.ptr(), inb.size());
  EVP_EncryptFinal(ctx, outb.ptr() + 4 + size1, &size2);
  *((int*)outb.ptr()) = size1 + size2;
  EVP_CIPHER_CTX_free(ctx);
  return outb;
}

Buffer AES_decrypt(const Buffer &key, const Buffer &iv, const Buffer &inb) {
  Buffer outb(inb.size());
  CHECK(outb.Allocate());
  int size1, size2;
  int size = *((int*)inb.ptr());
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  EVP_CIPHER_CTX_init(ctx);
  EVP_DecryptInit(ctx, EVP_aes_256_cbc(), key.ptr(), iv.ptr());
  EVP_DecryptUpdate(ctx, outb.ptr(), &size1, inb.ptr() + 4, size);
  EVP_DecryptFinal(ctx, outb.ptr() + size1, &size2);
  EVP_CIPHER_CTX_free(ctx);
  return outb;
}

}
