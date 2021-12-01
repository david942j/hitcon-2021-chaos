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

#define ASSERT_IOCTL_OK(fd, cmd, arg) do { \
  int ret = ioctl(fd, cmd, arg); \
  if (ret) { \
    error("%s#%d: expect ioctl(%s, %s, %s) to succeed, but got %s\n", \
          __func__, __LINE__, #fd, #cmd, #arg, strerror(errno)); \
  } \
} while (0)

static int OPEN(void) {
  int fd = open("/dev/chaos", O_RDWR);
  if (fd < 0)
    err(2, "open");
  return fd;
}

static void show_flag(void) {
  int fd = OPEN();
  struct chaos_request req = {
    .algo = CHAOS_ALGO_MD5,
    .input = 0,
    .in_size = 0,
    .output = 0x0,
    .out_size = 0x100,
  };
  ASSERT_IOCTL_OK(fd, CHAOS_ALLOCATE_BUFFER, 0x100);
  u_int8_t *buf = mmap(0, 0x100, PROT_WRITE, MAP_SHARED, fd, 0);
  assert(buf != MAP_FAILED);
  ASSERT_IOCTL_OK(fd, CHAOS_REQUEST, &req);
  puts(buf);
}

int main() {
  show_flag();
  return 0;
}
