/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <openssl/md5.h>

#define DEBUG

#ifdef DEBUG
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
    struct timeval tval = { 30, 0 };
    int ret = select(fd_ + 1, &readfds, NULL, NULL, &tval);
    // kills itself after 30 secs without an interrupt, to prevent becoming zombies
    if (ret == 0)
        exit(0);
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

class Inferior {
 public:
  Inferior(pid_t pid) : pid_(pid), last_status_(0) {}
  ~Inferior() {
    kill(pid_, SIGKILL);
    waitpid(pid_, NULL, 0);
  }

  void Attach() const {
    CHECK(ptrace(PTRACE_SEIZE, pid_, 0, PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD) == 0);
    CHECK(waitpid(pid_, NULL, 0) == pid_);
  }

  bool WaitForSys() {
    CHECK(ptrace(PTRACE_SYSEMU, pid_, 0, 0) == 0);
    CHECK(waitpid(pid_, &last_status_, 0) == pid_);
    return WIFSTOPPED(last_status_) && WSTOPSIG(last_status_) == (SIGTRAP | 0x80);
  }

  bool Read(uint8_t *ptr, uint32_t uptr, uint32_t size) const {
    for (uint32_t i = 0; i < size; i += sizeof(uint64_t)) {
      uint64_t data = ptrace(PTRACE_PEEKDATA, pid_, uptr + i, 0);
      *reinterpret_cast<uint64_t*>(ptr + i) = data;
      debug("[0x%x] = %lx\n", uptr + i, data);
    }
    return true;
  }

  bool Write(uint8_t *ptr, uint32_t uptr, uint32_t size) const {
    for (uint32_t i = 0; i < size; i += sizeof(uint64_t)) {
      if (ptrace(PTRACE_POKEDATA, pid_, uptr + i, *reinterpret_cast<uint64_t*>(ptr + i)))
        return false;
      debug("[0x%x] = %lx\n", uptr + i, *reinterpret_cast<uint64_t*>(ptr + i));
    }
    return true;
  }

  int lastStatus() const { return last_status_; }

 private:
  pid_t pid_;
  int last_status_;
};

class Buffer {
 public:
  Buffer(uint32_t size) : ptr_(nullptr), size_(size) {
    debug("called %x\n", size_);
  }
  ~Buffer() {
    debug("called %x\n", size_);
    if (ptr_)
      delete ptr_;
  }

  bool FromUser(Inferior &inferior, uint32_t uptr) {
    if (!ptr_ && !Allocate())
      return false;
    return inferior.Read(ptr_, uptr, size_);
  }

  bool ToUser(Inferior &inferior, uint32_t uptr) {
    CHECK(ptr_);
    return inferior.Write(ptr_, uptr, size_);
  }

  bool Allocate() {
    if (ptr_)
      return false;
    if (size_ == 0 || size_ > 0x100000)
      return false;
    // NOTE: can be a bug if size_ is not word-aligned
    // PEEKUSER can overflow
    ptr_ = new uint8_t[size_ + 8];
    if (!ptr_)
      return false;
    return true;
  }
  uint8_t *ptr() const { return ptr_; }
  uint32_t size() const { return size_; }

 private:
  uint8_t *ptr_;
  uint32_t size_;
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

#define SYS_chaos_crypto 0xc8a05
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
        if (req->out_size < MD5_DIGEST_LENGTH) {
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

}

namespace {

struct ptrace_syscall_info {
  uint8_t op;	/* PTRACE_SYSCALL_INFO_* */
  uint32_t arch __attribute__((__aligned__(sizeof(uint32_t))));
  uint64_t instruction_pointer;
  uint64_t stack_pointer;
  union {
    struct {
      uint64_t nr;
      uint64_t args[6];
    } entry;
    struct {
      int64_t rval;
      uint8_t is_error;
    } exit;
    struct {
      uint64_t nr;
      uint64_t args[6];
      uint32_t ret_data;
    } seccomp;
  };
};

#define SYS_exit 60
#define SYS_exit_group 231

long HandleCryptoCall(Inferior &inferior, uint64_t *args) {
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
      struct ptrace_syscall_info sys;
      CHECK(ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(sys), &sys) <= sizeof(sys));
      debug("op=%d 0x%x rip=0x%lx, NR=%ld\n", sys.op, sys.arch, sys.instruction_pointer, sys.entry.nr);
      if (sys.entry.nr == SYS_chaos_crypto) {
        auto res = HandleCryptoCall(inferior, sys.entry.args);
        debug("HandleCryptoCall returned %ld\n", res);
        CHECK(ptrace(PTRACE_POKEUSER, pid, offsetof(struct user_regs_struct, rax), res) == 0);
      } else if (sys.entry.nr == SYS_exit || sys.entry.nr == SYS_exit_group) {
        debug("firmware exited with %ld\n", sys.entry.args[0]);
        return sys.entry.args[0] == 0;
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
