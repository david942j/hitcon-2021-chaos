#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <chaos.h>

#define error(fmt, ...) do { fprintf(stderr, fmt, __VA_ARGS__); exit(2); } while (0)

#define ASSERT_IOCTL_ERR(fd, cmd, arg, err) do { \
  int ret = ioctl(fd, cmd, arg); \
  if (!ret) { \
    error("%s#%d: expect ioctl(%s, %s, %s) to return %s, but succeeded\n", \
          __func__, __LINE__, #fd, #cmd, #arg, #err); \
  } else if (errno != err) { \
    error("%s#%d: expect ioctl(%s, %s, %s) to return %s, but got %s\n", \
          __func__, __LINE__, #fd, #cmd, #arg, #err, strerror(errno)); \
  } \
} while (0)

#define ASSERT_IOCTL_OK(fd, cmd, arg) do { \
  int ret = ioctl(fd, cmd, arg); \
  if (ret) { \
    error("%s#%d: expect ioctl(%s, %s, %s) to succeed, but got %s\n", \
          __func__, __LINE__, #fd, #cmd, #arg, strerror(errno)); \
  } \
} while (0)

#define ASSERT_MMAP_ERR(size, prot, fd, off, err) do { \
  void *ptr = mmap(0, size, prot, MAP_SHARED, fd, off); \
  if (ptr != MAP_FAILED) { \
    error("%s#%d: expect mmap(%s, %s, %s, %s) to return %s, but succeeded\n", \
          __func__, __LINE__, #size, #prot, #fd, #off, #err); \
  } else if (errno != err) { \
    error("%s#%d: expect mmap(%s, %s, %s, %s) to return %s, but got %s\n", \
          __func__, __LINE__, #size, #prot, #fd, #off, #err, strerror(errno)); \
  } \
} while (0)

static int OPEN(void) {
  int fd = open("/dev/chaos", O_RDWR);
  if (fd < 0)
    err(2, "open");
  return fd;
}

static void test_alloate_buffer(void) {
  int fd = OPEN();
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 0ul, EINVAL);
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 1 << 20, ENOSPC);
  
  /* no buffer allocated */
  ASSERT_MMAP_ERR(0x1000, PROT_READ, fd, 0, EINVAL);

  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x1337);
  /* allocate twice should fail */
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 1, EEXIST);

  /* offset exceeds allocated size */
  ASSERT_MMAP_ERR(0x1000, PROT_READ, fd, 0x3000, EINVAL);

  static char buf[0x1000];
  void *ptr = mmap(0, sizeof(buf), PROT_WRITE, MAP_SHARED, fd, 0x1000);
  assert(ptr != MAP_FAILED);
  for(int i = 0; i < sizeof(buf); i++)
    buf[i] = i;
  memcpy(ptr, buf, sizeof(buf));
  munmap(ptr, sizeof(buf));
  ptr = mmap(0, sizeof(buf), PROT_READ, MAP_SHARED, fd, 0x1000);
  assert(ptr != MAP_FAILED);
  close(fd);
  assert(memcmp(ptr, buf, sizeof(buf)) == 0);
  munmap(ptr, sizeof(buf));
}

static void test_request(void) {
  int fd = OPEN();
  struct chaos_request req;
  req.algo = CHAOS_ALGO_ECHO;
  // buffer not allocated
  ASSERT_IOCTL_ERR(fd, CHAOS_REQUEST, &req, ENOSPC);

  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x2000);

  ASSERT_IOCTL_ERR(fd, CHAOS_REQUEST, 0x123, EFAULT);

  u_int8_t *in = mmap(0, 0x1000, PROT_WRITE, MAP_SHARED, fd, 0);
  void *out = mmap(0, 0x1000, PROT_READ, MAP_SHARED, fd, 0x1000);
  req.input = 0; req.in_size = 0x100;
  for (int i = 0; i < 0x100; i++)
    in[i] = i * 2;
  req.output = 0x1000; req.out_size = 0x1000;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == 0x100);
  assert(memcmp(in, out, 0x100) == 0);
  close(fd);
  munmap(in, 0x1000);
  munmap(out, 0x1000);
}

static void test_md5(void) {
  int fd = OPEN();
  struct chaos_request req = {
    .algo = CHAOS_ALGO_MD5,
    .input = 0,
    .in_size = 20,
    .output = 0x0,
    .out_size = 0x1,
  };
  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x100);
  u_int8_t *buf = mmap(0, 0x100, PROT_WRITE, MAP_SHARED, fd, 0);
  assert(buf != MAP_FAILED);
  for (int i = 0; i < req.in_size; i++)
    buf[i] = i * i;
  ASSERT_IOCTL_ERR(fd, CHAOS_REQUEST, &req, EPROTO);
  req.out_size = 0x100;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == 0x10);
  static const u_int8_t md5[] = { 223, 61, 195, 12, 93, 221, 69, 55, 239, 193, 1, 123, 88, 214, 210, 237 };
  assert(memcmp(buf, md5, 0x10) == 0);
  close(fd);
  munmap(buf, 0x100);
}

static void test_aes(void) {
  int fd = OPEN();
  struct chaos_request req = {
    .algo = CHAOS_ALGO_AES_ENC,
    .input = 0x0,
    .in_size = 32,
    .key = 0x100,
    .key_size = 16,
    .output = 0x0,
    .out_size = 0x100,
  };
  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x200);
  u_int8_t *buf = mmap(0, 0x200, PROT_WRITE, MAP_SHARED, fd, 0);
  assert(buf != MAP_FAILED);
  u_int8_t *key = buf + 0x100;
  for (int i = 0; i < req.in_size; i++)
    buf[i] = i * 2;
  for (int i = 0; i < req.key_size; i++)
    key[i] = i * 3;
  req.out_size = 0x100;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == 0x20);
  static const u_int8_t aes_enc[] = { 239, 188, 210, 239, 56, 119, 92, 235, 167, 240, 183, 215, 118, 73, 171, 224, 2, 24, 97, 202, 206, 41, 229, 16, 124, 160, 169, 35, 161, 180, 226, 185 };
  assert(memcmp(buf, aes_enc, 0x20) == 0);
  req.algo = CHAOS_ALGO_AES_DEC;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == 0x20);
  static const u_int8_t aes_dec[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 };
  assert(memcmp(buf, aes_dec, 0x10) == 0);
  close(fd);
  munmap(buf, 0x200);
}

static void test_rc4(void) {
  int fd = OPEN();
  struct chaos_request req = {
    .algo = CHAOS_ALGO_RC4_ENC,
    .input = 0x0,
    .in_size = 20,
    .key = 0x100,
    .key_size = 20,
    .output = 0x0,
    .out_size = 0x100,
  };
  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x200);
  u_int8_t *buf = mmap(0, 0x200, PROT_WRITE, MAP_SHARED, fd, 0);
  assert(buf != MAP_FAILED);
  u_int8_t *key = buf + 0x100;
  for (int i = 0; i < req.in_size; i++)
    buf[i] = i * 2;
  for (int i = 0; i < req.key_size; i++)
    key[i] = i * 3;
  req.out_size = 0x100;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == req.in_size);
  static const u_int8_t rc4_enc[] = { 12, 65, 89, 68, 192, 125, 242, 250, 254, 45, 116, 140, 194, 252, 1, 4, 27, 33, 74, 61 };
  assert(memcmp(buf, rc4_enc, 0x14) == 0);
  req.algo = CHAOS_ALGO_RC4_DEC;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == req.in_size);
  static const u_int8_t rc4_dec[] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38 };
  assert(memcmp(buf, rc4_dec, 0x14) == 0);
  close(fd);
  munmap(buf, 0x200);
}

static void test_bf(void) {
  int fd = OPEN();
  struct chaos_request req = {
    .algo = CHAOS_ALGO_BF_ENC,
    .input = 0x0,
    .in_size = 8,
    .key = 0x100,
    .key_size = 12,
    .output = 0x0,
    .out_size = 0x100,
  };
  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x200);
  u_int8_t *buf = mmap(0, 0x200, PROT_WRITE, MAP_SHARED, fd, 0);
  assert(buf != MAP_FAILED);
  u_int8_t *key = buf + 0x100;
  for (int i = 0; i < req.in_size; i++)
    buf[i] = i * 2;
  for (int i = 0; i < req.key_size; i++)
    key[i] = i * 3;
  req.out_size = 0x100;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == req.in_size);
  static const u_int8_t bf_enc[] = { 199, 137, 205, 44, 93, 170, 122, 90 };
  assert(memcmp(buf, bf_enc, 0x8) == 0);
  req.algo = CHAOS_ALGO_BF_DEC;
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  assert(req.out_size == req.in_size);
  static const u_int8_t bf_dec[] = { 0, 2, 4, 6, 8, 10, 12, 14 };
  assert(memcmp(buf, bf_dec, 0x8) == 0);
  close(fd);
  munmap(buf, 0x200);
}

int main() {
  test_alloate_buffer();
  test_request();
  test_md5();
  test_aes();
  test_rc4();
  test_bf();
  return 0;
}
