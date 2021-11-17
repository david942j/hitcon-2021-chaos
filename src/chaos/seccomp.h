// gem install seccomp-tools
// sed '/^#define/r'<(echo;seccomp-tools asm sandbox.bpf -f c_source) seccomp.h.tpl > seccomp.h
#ifndef _SECCOMP_H
#define _SECCOMP_H

#include <linux/seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>

static void install_seccomp() {
  static unsigned char filter[] = {32,0,0,0,4,0,0,0,21,0,0,22,62,0,0,192,32,0,0,0,0,0,0,0,53,0,20,0,0,0,0,64,21,0,18,0,0,0,0,0,21,0,17,0,1,0,0,0,21,0,16,0,3,0,0,0,21,0,15,0,9,0,0,0,21,0,14,0,11,0,0,0,21,0,13,0,12,0,0,0,21,0,12,0,56,0,0,0,21,0,11,0,101,0,0,0,21,0,10,0,61,0,0,0,21,0,9,0,17,1,0,0,21,0,8,0,14,0,0,0,21,0,7,0,39,0,0,0,21,0,6,0,186,0,0,0,21,0,5,0,62,0,0,0,21,0,4,0,234,0,0,0,21,0,3,0,23,0,0,0,21,0,2,0,60,0,0,0,21,0,1,0,231,0,0,0,6,0,0,0,0,0,0,0,6,0,0,0,0,0,255,127,6,0,0,0,0,0,0,0};
  struct prog {
    unsigned short len;
    unsigned char *filter;
  } rule = {
    .len = sizeof(filter) >> 3,
    .filter = filter
  };
  if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) { perror("prctl(PR_SET_NO_NEW_PRIVS)"); exit(2); }
  if(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &rule) < 0) { perror("prctl(PR_SET_SECCOMP)"); exit(2); }
}

#endif // _SECCOMP_H
