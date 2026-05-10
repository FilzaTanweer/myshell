/*
 * include/my_syscalls.h
 *
 * Direct kernel syscall wrappers that bypass glibc.
 * Required by proposal: "implement 2-3 system call interfaces using
 * direct syscall() wrappers to avoid relying solely on glibc wrappers."
 *
 * Syscall numbers for x86-64 Linux:
 *   SYS_write  = 1
 *   SYS_getpid = 39
 *   SYS_fork   = 57
 */

#ifndef MY_SYSCALLS_H
#define MY_SYSCALLS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>     /* PATH_MAX */

/*
 * my_write() — write 'count' bytes from 'buf' to file descriptor 'fd'.
 * Used in signal handlers because write() is async-signal-safe,
 * unlike printf() which uses internal locks that can deadlock.
 */
static inline ssize_t my_write(int fd, const void *buf, size_t count)
{
    return (ssize_t) syscall(SYS_write, fd, buf, count);
}

/*
 * my_getpid() — return the PID of the calling process.
 * Demonstrated in the startup banner to verify direct kernel access.
 */
static inline pid_t my_getpid(void)
{
    return (pid_t) syscall(SYS_getpid);
}

/*
 * my_fork() — create a child process by duplicating the calling process.
 * Returns > 0 in parent (child PID), 0 in child, < 0 on failure.
 * Called by execute_command() instead of the standard glibc fork().
 */
static inline pid_t my_fork(void)
{
    return (pid_t) syscall(SYS_fork);
}

/*
 * my_puts() — write a C-string to STDOUT via my_write().
 * Convenience wrapper used for the prompt and banner output.
 */
static inline void my_puts(const char *s)
{
    size_t len = 0;
    while (s[len]) len++;
    my_write(1, s, len);
}

#endif /* MY_SYSCALLS_H */
