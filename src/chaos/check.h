/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _CHECK_H
#define _CHECK_H

#include <unistd.h>

// #define DEBUG

#ifdef DEBUG
#include <cstdio>
#define debug(fmt, ...) fprintf(stderr, "%s:%d: %s: " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define debug(...)
#endif

#define CHECK(cond) do { \
  if (!(cond)) { \
    debug("check \"%s\" failed.\n", #cond); \
    _exit(2); \
  } \
} while (0)

#endif // _CHECK_H
