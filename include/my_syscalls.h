/*
 * include/my_syscalls.h
 *
 * Low-level syscall wrappers using the syscall() interface.
 * These bypass glibc wrappers and invoke the Linux kernel directly.
 *
 * Required by the updated proposal:
 *   "implement 2-3 system call interfaces using inline assembly
 *    or direct syscall() wrappers to avoid relying solely on
 *    standard glibc wrappers."
 *
 * Syscall numbers for x86-64 Linux (from <sys/syscall.h>):
 *   SYS_write  = 1
 *   SYS_getpid = 39
 *   SYS_fork   = 57
 */

#ifndef MY_SYSCALLS_H
#define MY_SYSCALLS_H

/* _GNU_SOURCE required for syscall() prototype on some glibc versions */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/syscall.h>   /* SYS_write, SYS_getpid, SYS_fork */
#include <unistd.h>        /* syscall() prototype              */
#include <sys/types.h>     /* pid_t, ssize_t                   */

/*
 * my_write() — writes 'count' bytes from 'buf' to file descriptor 'fd'.
 *
 * Equivalent to:  write(fd, buf, count)
 * Kernel syscall: SYS_write (syscall number 1 on x86-64)
 *
 * Returns number of bytes written, or -1 on error.
 */
static inline ssize_t my_write(int fd, const void *buf, size_t count)
{
    return (ssize_t) syscall(SYS_write, fd, buf, count);
}

/*
 * my_getpid() — returns the PID of the calling process.
 *
 * Equivalent to:  getpid()
 * Kernel syscall: SYS_getpid (syscall number 39 on x86-64)
 */
static inline pid_t my_getpid(void)
{
    return (pid_t) syscall(SYS_getpid);
}

/*
 * my_fork() — creates a child process by duplicating the calling process.
 *
 * Equivalent to:  fork()
 * Kernel syscall: SYS_fork (syscall number 57 on x86-64)
 *
 * Returns:
 *   > 0  in parent  (child PID)
 *   = 0  in child
 *   < 0  on failure
 *
 * NOTE: On modern Linux, SYS_fork internally calls clone(). This wrapper
 * still demonstrates the direct syscall interface as required.
 */
static inline pid_t my_fork(void)
{
    return (pid_t) syscall(SYS_fork);
}

/*
 * Convenience: write a C-string to STDOUT using my_write().
 * Used for the shell prompt so even the prompt bypasses printf/glibc.
 */
static inline void my_puts(const char *s)
{
    size_t len = 0;
    while (s[len]) len++;
    my_write(1, s, len);
}

#endif /* MY_SYSCALLS_H */
