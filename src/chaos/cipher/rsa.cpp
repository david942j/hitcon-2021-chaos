/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 lyc
 */

#include "rsa.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <gmp.h>

namespace rsa {

void encrypt(uint8_t *N, size_t nlen, uint8_t *E, size_t elen, uint8_t *inb, size_t len, uint8_t *outb) {
  mpz_t in, result, e, n;
  mpz_init(in);
  mpz_init(result);
  mpz_init(e);
  mpz_init(n);
  mpz_import(in, len, -1, 1, 0, 0, inb);
  mpz_import(e, elen, -1, 1, 0, 0, E);
  mpz_import(n, nlen, -1, 1, 0, 0, N);
  mpz_powm(result, in, e, n);
  size_t size;
  mpz_export(outb, &size, -1, 1, 0, 0, result);
  assert(size <= nlen);
  mpz_clear(n);
  mpz_clear(e);
  mpz_clear(result);
  mpz_clear(in);
}

}
