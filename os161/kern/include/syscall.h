#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#include <kern/unistd.h>
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */


int sys_reboot(int code);

int sys_write(int fd, const void *buf, size_t nbytes);

int sys_read(int fd, void *buf, size_t buflen);

pid_t sys_fork(struct trapframe *tf, int *retval);

pid_t sys_getpid();

void sys_exit();

pid_t sys_waitpid(pid_t PID, int *status, int options, int *retval);

int sys_execv(const char *program, char **args, int* retval);
static void kfree_all(char *argv[]);

time_t sys__time(time_t *seconds, unsigned long *nanoseconds, int* retval);

void* sys_sbrk(intptr_t amount, int* retval);



#endif /* _SYSCALL_H_ */