/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#ifndef _INFERIOR_H
#define _INFERIOR_H

#include <sys/types.h>

#include <cstdint>

#define SYS_exit 60
#define SYS_exit_group 231
#define SYS_chaos_crypto 0xc8a05

class Inferior {
 public:
  Inferior(pid_t pid) : pid_(pid), last_status_(0) {}
  ~Inferior();

  void Attach() const;
  void SetContext(void *pc, void *stk) const;

  bool WaitForSys();
  void SetSyscallRet(long retval) const;

  bool Read(uint8_t *ptr, uint32_t uptr, uint32_t size) const;
  bool Write(uint8_t *ptr, uint32_t uptr, uint32_t size) const;

  inline uint64_t sysnr() const { return sys_.entry.nr; }
  inline const uint64_t *sysargs() const { return sys_.entry.args; }
  inline bool IsSysCrpyto() const { return sysnr() == SYS_chaos_crypto; }
  inline bool IsExit() const { return sysnr() == SYS_exit || sysnr() == SYS_exit_group; }
  inline int lastStatus() const { return last_status_; }

 private:
  pid_t pid_;
  int last_status_;
  struct {
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
  } sys_;
};

#endif // _INFERIOR_H
