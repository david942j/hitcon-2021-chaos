#include "syscall.h"

void handle_mailbox(void);

void _start()
{
    handle_mailbox();
    syscall(SYS_exit, 0);
}
