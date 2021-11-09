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

#include <chaos.h>

typedef unsigned long long __u64;

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

static void test_alloate_buffer(void) {
  int fd = open("/dev/chaos", O_RDWR);
  int ret;
  if (fd < 0)
    err(2, "open");
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 0ul, EINVAL);
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 1 << 20, ENOSPC);
  
  /* no buffer allocated */
  ASSERT_MMAP_ERR(0x1000, PROT_READ, fd, 0, EINVAL);

  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x1337);
  /* allocate twice should fail */
  ASSERT_IOCTL_ERR(fd, CHAOS_ALLOCATE_BUFFER, 1, EEXIST);

  /* offset exceeds allocated size */
  ASSERT_MMAP_ERR(0x1000, PROT_READ, fd, 0x3000, EINVAL);

  void *ptr = mmap(0, 4096, PROT_WRITE, MAP_SHARED, fd, 0x1000);
  assert(ptr != MAP_FAILED);
  static char buf[4096];
  for(int i = 0; i < sizeof(buf); i++)
    buf[i] = i;
  memcpy(ptr, buf, 4096);
  munmap(ptr, 4096);
  ptr = mmap(0, 4096, PROT_READ, MAP_SHARED, fd, 0x1000);
  assert(ptr != MAP_FAILED);
  assert(memcmp(ptr, buf, 4096) == 0);
}

int main() {
  test_alloate_buffer();
  return 0;
}
