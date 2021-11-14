/*
 * CHAOS - CryptograpHy AcceleratOr Silicon
 *
 * Copyright (c) 2021 david942j
 */

#include "inferior.h"

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <csignal>
#include <cstddef>
#include <cstdint>

#include "check.h"

Inferior::~Inferior() {
  kill(pid_, SIGKILL);
  waitpid(pid_, nullptr, 0);
}

void Inferior::Attach() const {
  CHECK(ptrace(PTRACE_SEIZE, pid_, 0, PTRACE_O_EXITKILL | PTRACE_O_TRACESYSGOOD) == 0);
  CHECK(waitpid(pid_, nullptr, 0) == pid_);
}

void Inferior::SetContext(void *pc, void *stk) const {
  struct user_regs_struct regs;
  CHECK(ptrace(PTRACE_GETREGS, pid_, 0, &regs) == 0);
  debug("%p %p\n", pc, stk);
  regs.rip = reinterpret_cast<uint64_t>(pc);
  regs.rsp = reinterpret_cast<uint64_t>(stk);
  CHECK(ptrace(PTRACE_SETREGS, pid_, 0, &regs) == 0);
}

bool Inferior::WaitForSys() {
  CHECK(ptrace(PTRACE_SYSEMU, pid_, 0, 0) == 0);
  CHECK(waitpid(pid_, &last_status_, 0) == pid_);
  if (!(WIFSTOPPED(last_status_) && WSTOPSIG(last_status_) == (SIGTRAP | 0x80)))
    return false;
  CHECK(ptrace(PTRACE_GET_SYSCALL_INFO, pid_, sizeof(sys_), &sys_) <= sizeof(sys_));
  debug("op=%d 0x%x rip=0x%lx, NR=%ld\n", sys_.op, sys_.arch, sys_.instruction_pointer, sys_.entry.nr);
  return true;
}

bool Inferior::Read(uint8_t *ptr, uint32_t uptr, uint32_t size) const {
  for (uint32_t i = 0; i < size; i += sizeof(uint64_t)) {
    uint64_t data = ptrace(PTRACE_PEEKDATA, pid_, uptr + i, 0);
    *reinterpret_cast<uint64_t*>(ptr + i) = data;
    debug("[0x%x] = %lx\n", uptr + i, data);
  }
  return true;
}

bool Inferior::Write(uint8_t *ptr, uint32_t uptr, uint32_t size) const {
  for (uint32_t i = 0; i < size; i += sizeof(uint64_t)) {
    if (ptrace(PTRACE_POKEDATA, pid_, uptr + i, *reinterpret_cast<uint64_t*>(ptr + i)))
      return false;
    debug("[0x%x] = %lx\n", uptr + i, *reinterpret_cast<uint64_t*>(ptr + i));
  }
  return true;
}

void Inferior::SetSyscallRet(long retval) const {
  CHECK(ptrace(PTRACE_POKEUSER, pid_, offsetof(struct user_regs_struct, rax), retval) == 0);
}
