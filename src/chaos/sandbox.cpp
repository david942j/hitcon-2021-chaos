/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/select.h>

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "buffer.h"
#include "check.h"
#include "crypto.h"
#include "inferior.h"

namespace {

class MemoryRegion {
 public:
  MemoryRegion(uint64_t base, uint64_t size, int prot) : base_((void *)base), size_(size) {
    auto ptr = mmap(base_, size_, prot, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    CHECK(ptr == base_);
  }

  MemoryRegion(int fd, uint64_t base) : base_((void *)base) {
    lseek(fd, 0, SEEK_SET);
    size_ = lseek(fd, 0, SEEK_END);
    auto ptr = mmap(base_, size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    CHECK(ptr == base_);
    close(fd);
  }

  ~MemoryRegion() {
    munmap(base_, size_);
  }

  inline uint64_t size() const { return size_; }

  uint64_t Read64(uint64_t offset) const {
    CHECK(offset <= size_ - 8);
    return *static_cast<uint64_t *>(base_ + offset);
  }

  void Write64(uint64_t offset, uint64_t val) const {
    CHECK(offset <= size_ - 8);
    *static_cast<uint64_t *>(base_ + offset) = val;
  }

  void *at(uint64_t offset) const {
      CHECK(offset < size_);
      return base_ + offset;
  }

 private:
  void *base_;
  uint64_t size_;
};

class Event {
 public:
  Event(int fd) : fd_(fd) {
    CHECK(fd < FD_SETSIZE);
  }
  ~Event() {
    close(fd_);
  }

  void WaitAndClear() const {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd_, &readfds);
    int ret = select(fd_ + 1, &readfds, NULL, NULL, NULL);
    CHECK(ret == 1);
    uint64_t dummy;
    ret = read(fd_, &dummy, sizeof(dummy));
    CHECK(ret == sizeof(dummy));
  }

  void Trigger() const {
    uint64_t one;
    int ret = write(fd_, &one, sizeof(one));
    CHECK(ret == sizeof(one));
  }

 private:
  int fd_;
};

constexpr int kCsrFd = 3;
constexpr int kDramFd = 4;
constexpr uint64_t kCsrBase   = 0x00010000;
constexpr uint64_t kDramBase  = 0x10000000;
constexpr uint64_t kCodeBase  = 0x00100000;
constexpr uint64_t kCodeSize  = 0x00100000; // 1MB
constexpr uint64_t kStackBase = 0x00201000;
constexpr uint64_t kStackSize = 0x00002000;

MemoryRegion CSR(kCsrFd, kCsrBase);
MemoryRegion DRAM(kDramFd, kDramBase);
MemoryRegion Code(kCodeBase, kCodeSize, PROT_READ | PROT_WRITE | PROT_EXEC);
MemoryRegion Stack(kStackBase, kStackSize, PROT_READ | PROT_WRITE);

} // namespace

// TODO: move to "firmware"
namespace firmware {

enum chaos_command_code {
	CHAOS_CMD_CODE_REQUEST,
};

struct chaos_mailbox_cmd {
    uint32_t seq;
    enum chaos_command_code code;
    uint32_t dma_addr;
    uint32_t dma_size;
};

struct chaos_mailbox_rsp {
    uint32_t seq;
    uint32_t retval;
};

enum chaos_request_algo {
	/* copy input to output, for testing purpose */
	CHAOS_ALGO_ECHO,
	CHAOS_ALGO_MD5,
};

struct chaos_request {
	enum chaos_request_algo algo;
	uint32_t input;
	uint32_t in_size;
	uint32_t output;
	uint32_t out_size;
};

struct Csrs {
    uint64_t version; /* R */
    uint64_t cmdq_addr; /* RW */
    uint64_t rspq_addr; /* RW */
    uint64_t cmdq_size; /* RW */
    uint64_t rspq_size; /* RW */
    uint64_t reset; /* W */
    uint64_t irq_status; /* R */
    uint64_t clear_irq; /* W */
    uint64_t cmd_sent; /* W */
    uint64_t cmd_head; /* R */
    uint64_t cmd_tail; /* W */
    uint64_t rsp_head; /* W */
    uint64_t rsp_tail; /* R */
    uint64_t reserved[3];
};

#define READ_CSR(field) CSR.Read64(offsetof(Csrs, field))
#define WRITE_CSR(field, val) CSR.Write64(offsetof(Csrs, field), val)

static inline void *VALID_ADDR_SIZE(uint32_t addr, uint32_t sz) {
    CHECK(sz <= DRAM.size());
    CHECK(addr < DRAM.size());
    CHECK(addr <= DRAM.size() - sz);
    return DRAM.at(addr);
}

static uint64_t real_index(uint64_t idx, uint64_t size)
{
    return idx & (size - 1);
}

#define SYS_chaos_crypto 0xc8a05
static int handle_cmd_request(struct chaos_mailbox_cmd *cmd)
{
    struct chaos_request *req;
    void *in, *out;

    // XXX: the implementation here is bad, kernel is able to hack with TOCTOU
    CHECK(cmd->dma_size == sizeof(struct chaos_request));
    req = (chaos_request*)VALID_ADDR_SIZE(cmd->dma_addr, cmd->dma_size);
    in = VALID_ADDR_SIZE(req->input, req->in_size);
    out = VALID_ADDR_SIZE(req->output, req->out_size);

    switch (req->algo) {
    case CHAOS_ALGO_ECHO:
        memcpy(out, in, req->in_size);
        return req->in_size;
    case CHAOS_ALGO_MD5:
        if (req->out_size < 0x10) {
            // TODO: add logs?
            return -EOVERFLOW;
        }
        return syscall(SYS_chaos_crypto, CHAOS_ALGO_MD5, in, req->in_size, out);
    default:
        CHECK(false);
        return 0;
    }
}

static int handle_cmd(struct chaos_mailbox_cmd *cmd)
{
    CHECK(cmd->code == CHAOS_CMD_CODE_REQUEST);
    return handle_cmd_request(cmd);
}

static inline uint64_t queue_inc(uint64_t val, uint64_t size)
{
    return (++val) & ((size << 1) - 1);
}

static void push_rsp(struct chaos_mailbox_rsp *rsp)
{
    struct chaos_mailbox_rsp *queue = static_cast<chaos_mailbox_rsp*>(DRAM.at(READ_CSR(rspq_addr)));
    const uint64_t rspq_size = READ_CSR(rspq_size);
    uint64_t tail = READ_CSR(rsp_tail);

    queue[real_index(tail, rspq_size)] = *rsp;
    WRITE_CSR(rsp_tail, queue_inc(tail, rspq_size));
}


void HandleMailbox() {
    const uint64_t cmdq_size = READ_CSR(cmdq_size);
    uint64_t head = READ_CSR(cmd_head);
    uint64_t tail = READ_CSR(cmd_tail);
    struct chaos_mailbox_cmd *cmdq = static_cast<chaos_mailbox_cmd*>(DRAM.at(READ_CSR(cmdq_addr)));
    struct chaos_mailbox_cmd *cmd;
    struct chaos_mailbox_rsp rsp;

    if (head == tail)
        return;
    cmd = &cmdq[real_index(head, cmdq_size)];
    WRITE_CSR(cmd_head, queue_inc(head, cmdq_size));
    rsp.retval = handle_cmd(cmd);
    rsp.seq = cmd->seq;
    push_rsp(&rsp);
}

} // namespace firmware

namespace {

long HandleCryptoCall(Inferior &inferior, const uint64_t *args) {
  switch (args[0]) {
  case firmware::CHAOS_ALGO_MD5: {
    uint32_t in = args[1];
    uint32_t in_size = args[2];
    uint32_t out = args[3];
    Buffer inb(in_size);
    if (!inb.FromUser(inferior, in))
      return -EFAULT;
    Buffer outb(crypto::MD5(inb));
    if (!outb.ToUser(inferior, out))
      return -EFAULT;
    return outb.size();
  }
  default:
    return -ENOSYS;
  }
}

bool Sandboxing() {
  pid_t pid = fork();
  if (!pid) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    raise(SIGSTOP);
    // TODO: jmp to Code
    firmware::HandleMailbox();
    exit(0);
  }

  Inferior inferior(pid);
  inferior.Attach();
  while (1) {
    if (inferior.WaitForSys()) {
      if (inferior.IsSysCrpyto()) {
        auto res = HandleCryptoCall(inferior, inferior.sysargs());
        debug("HandleCryptoCall returned %ld\n", res);
        inferior.SetSyscallRet(res);
      } else if (inferior.IsExit()) {
        debug("firmware exited with %ld\n", inferior.sysargs()[0]);
        return inferior.sysargs()[0] == 0;
      }
    } else {
      debug("firmware crashed with 0x%x\n", inferior.lastStatus());
      return false;
    }
  }
}

void RunMain() {
  constexpr int kEventFdFromHost = 5;
  constexpr int kEventFdToHost = 6;
  Event from(kEventFdFromHost), to(kEventFdToHost);

  // TODO: take the first event as firmware loading request
  while (1) {
    from.WaitAndClear();
    if (Sandboxing())
      to.Trigger();
  }
}

} // namespace

int main() {
  RunMain();
  return 0;
}
