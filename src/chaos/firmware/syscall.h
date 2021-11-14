#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_exit 60

#define SYS_chaos_crypto 0xc8a05

long syscall(int sysnr, ...);

#endif /* _SYSCALL_H */
