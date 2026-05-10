/*
 * src/myshell.c — Custom Command Line Shell (myshell)
 *
 * FAST-NUCES | Operating Systems Project
 * Instructor : Minhal Raza
 * Authors    : Filza Tanweer  (24k-0708)
 *              Fatima Hiader  (24k-0551)
 *              Bismah Sheikh  (24k-0795)
 *
 * WBS Phase Mapping:
 *   Phase 1 — Lexical Analysis         → parser.c / parser.h
 *   Phase 2 — Process Control          → execute_command()
 *   Phase 3 — Low-Level Syscalls       → my_syscalls.h
 *   Phase 4 — Features & Redirection   → execute_command() + signal handlers
 *   Phase 5 — Testing & Documentation  → tests/run_tests.sh
 *
 * OS Concepts Demonstrated:
 *   fork()        — duplicates address space (parent-child creation)
 *   execvp()      — replaces process image (program loading)
 *   waitpid()     — parent-child synchronisation, zombie prevention
 *   sigaction()   — signal interception (Ctrl+C handling)
 *   sigprocmask() — signal mask management in child processes
 *   dup2()        — file descriptor manipulation for I/O redirection
 *   syscall()     — direct kernel interface bypassing glibc
 *   chdir()       — directory change (why cd must be a built-in)
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>     /* PATH_MAX */

#include "../include/my_syscalls.h"
#include "../include/history.h"
#include "../include/parser.h"

/* ANSI colour codes for the coloured prompt */
#define CLR_GREEN  "\033[1;32m"
#define CLR_BLUE   "\033[1;34m"
#define CLR_RESET  "\033[0m"

/* Global history list — singly linked list with tail pointer */
static history_list_t g_history;

/* ════════════════════════════════════════════════════════════════════
 * PHASE 3 — LOW-LEVEL SYSCALL DEMO
 * Startup banner uses my_write() and my_getpid(), both of which
 * invoke the kernel directly via syscall(), bypassing glibc.
 * ════════════════════════════════════════════════════════════════════ */
static void print_banner(void)
{
    pid_t pid = my_getpid();   /* syscall(SYS_getpid) — direct kernel call */

    char banner[1024];
    int  n = snprintf(banner, sizeof(banner),
        "\n  myshell -- Custom Unix Shell | FAST-NUCES OS Project\n"
        "  Authors : Filza Tanweer, Fatima Haider, Bismah Sheikh\n"
        "  Shell PID (via my_getpid syscall): %d\n"
        "  Type 'help' for commands. Type 'exit' to quit.\n\n",
        pid);

    my_write(STDOUT_FILENO, banner, (size_t)n);  /* syscall(SYS_write) */
}

/* ════════════════════════════════════════════════════════════════════
 * PHASE 4 — SIGNAL HANDLERS
 *
 * SIGINT (Ctrl+C):
 *   Shell catches and ignores it (only prints a newline).
 *   Child processes have SIG_DFL restored before exec(), so they
 *   terminate normally from Ctrl+C while the shell survives.
 *   my_write() is used here because it is async-signal-safe;
 *   printf() uses internal locks and can deadlock in signal handlers.
 *
 * SIGCHLD:
 *   Delivered by the kernel when any child process exits.
 *   Calls waitpid(-1, NULL, WNOHANG) in a loop to reap all finished
 *   background children — this prevents zombie processes.
 *   errno is saved/restored because waitpid() modifies it.
 * ════════════════════════════════════════════════════════════════════ */
static void handle_sigint(int sig)
{
    (void)sig;
    my_write(STDOUT_FILENO, "\n", 1);   /* async-signal-safe */
}

static void handle_sigchld(int sig)
{
    (void)sig;
    int saved_errno = errno;
    /*
     * WNOHANG: return immediately if no child has exited yet.
     * Loop because multiple children may finish simultaneously.
     */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

static void install_signal_handlers(void)
{
    struct sigaction sa_int, sa_chld;

    /* SIGINT: custom handler, SA_RESTART resumes interrupted fgets() */
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    /* SIGCHLD: reap children, SA_NOCLDSTOP avoids notification on stop */
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

/* ════════════════════════════════════════════════════════════════════
 * PHASE 2 + PHASE 4 — PROCESS CONTROL + I/O REDIRECTION
 *
 * execute_command() — the fork-exec-wait loop.
 *
 * Step 1: my_fork() — direct syscall(SYS_fork), duplicates process.
 * Step 2: CHILD restores signal defaults, unblocks all signals.
 * Step 3: CHILD sets up I/O redirection via open() + dup2().
 *         dup2(fd, STDOUT_FILENO) makes fd 1 point to our file;
 *         the execvp() program then writes to the file transparently.
 * Step 4: CHILD calls execvp() — replaces its image with the program.
 * Step 5: PARENT calls waitpid() (foreground) or returns (background).
 *         Background children are reaped by the SIGCHLD handler.
 * ════════════════════════════════════════════════════════════════════ */
static void execute_command(const cmd_t *cmd)
{
    pid_t pid = my_fork();   /* Phase 3: direct kernel fork syscall */

    if (pid < 0) {
        perror("my_fork");
        return;
    }

    /* ── CHILD process ── */
    if (pid == 0) {
        /*
         * Restore default signal dispositions in child.
         * If SIGINT stays customised, the child would ignore Ctrl+C.
         * SIG_DFL means the child dies from SIGINT normally.
         */
        signal(SIGINT,  SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /*
         * Unblock all signals. The parent shell may have some signals
         * masked; children must receive signals (e.g., Ctrl+C) normally.
         */
        sigset_t mask;
        sigemptyset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        /* ── Input redirection (<) ── */
        if (cmd->infile) {
            int fd = open(cmd->infile, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n",
                        cmd->infile, strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);   /* fd no longer needed after dup2 */
        }

        /* ── Output redirection (> or >>) ── */
        if (cmd->outfile) {
            int flags = O_WRONLY | O_CREAT |
                        (cmd->append ? O_APPEND : O_TRUNC);
            int fd = open(cmd->outfile, flags, 0644);
            if (fd < 0) {
                fprintf(stderr, "myshell: %s: %s\n",
                        cmd->outfile, strerror(errno));
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);   /* fd no longer needed after dup2 */
        }

        /*
         * execvp() searches $PATH for the command name.
         * Supports: absolute paths (/bin/ls), relative (./prog),
         * and PATH-searched names (ls, echo, cat...).
         * On success, this call does NOT return.
         */
        execvp(cmd->args[0], cmd->args);

        /* Only reached if execvp() fails */
        fprintf(stderr, "myshell: '%s': %s\n",
                cmd->args[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* ── PARENT process ── */
    if (!cmd->background) {
        /*
         * waitpid() blocks until the foreground child finishes.
         * Collecting the exit status prevents the child from
         * becoming a zombie in the process table.
         */
        int status;
        waitpid(pid, &status, 0);

        if (WIFSIGNALED(status)) {
            /* Child was killed by a signal (e.g. Ctrl+C → SIGINT=2) */
            fprintf(stderr, "\n[process terminated by signal %d]\n",
                    WTERMSIG(status));
        }
    } else {
        /*
         * Background (&): parent returns immediately.
         * The SIGCHLD handler reaps this child when it finishes.
         */
        char msg[64];
        int  n = snprintf(msg, sizeof(msg),
                     "[%d] started in background\n", pid);
        my_write(STDOUT_FILENO, msg, (size_t)n);
    }
}

/* ════════════════════════════════════════════════════════════════════
 * BUILT-IN COMMANDS — Phase 3 ("without using system()")
 *
 * cd must be a built-in because chdir() only affects the calling
 * process. If we forked a child to run cd, only the child's working
 * directory would change; the shell's own directory would be unchanged.
 * ════════════════════════════════════════════════════════════════════ */
static void builtin_cd(const cmd_t *cmd)
{
    const char *target;

    if (cmd->argc > 1) {
        target = cmd->args[1];
    } else {
        target = getenv("HOME");
        if (target == NULL) target = "/";
    }

    if (chdir(target) != 0)
        perror("cd");
}

static void builtin_pwd(void)
{
    char cwd[PATH_MAX];   /* PATH_MAX from <limits.h> — handles deep paths */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("pwd");
        return;
    }
    size_t len = strlen(cwd);
    cwd[len]   = '\n';
    my_write(STDOUT_FILENO, cwd, len + 1);
}

static void builtin_help(void)
{
    static const char *help =
        "\n  ── myshell built-in commands ──────────────────────────\n"
        "  cd [dir]        Change directory (default: $HOME)\n"
        "  pwd             Print current working directory\n"
        "  history         Show command history (linked list)\n"
        "  exit            Exit the shell (frees all memory)\n"
        "  help            Show this help message\n\n"
        "  ── execution features ─────────────────────────────────\n"
        "  cmd &           Run command in background (SIGCHLD reap)\n"
        "  cmd > file      Redirect stdout to file (truncate)\n"
        "  cmd >> file     Redirect stdout to file (append)\n"
        "  cmd < file      Redirect stdin from file\n"
        "  /abs/path       Execute via absolute path\n"
        "  ./rel/path      Execute via relative path\n\n"
        "  ── low-level syscall wrappers (Phase 3) ───────────────\n"
        "  my_fork()    →  syscall(SYS_fork)   [used in execute_command]\n"
        "  my_write()   →  syscall(SYS_write)  [used for prompt & banner]\n"
        "  my_getpid()  →  syscall(SYS_getpid) [shown at startup]\n"
        "  ─────────────────────────────────────────────────────────\n\n";
    my_write(STDOUT_FILENO, help, strlen(help));
}

/* ════════════════════════════════════════════════════════════════════
 * PROMPT — printed via my_write() (direct syscall) for Phase 3 demo.
 * Uses PATH_MAX buffer to handle arbitrarily deep directory paths.
 * ════════════════════════════════════════════════════════════════════ */
static void print_prompt(void)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");

    char prompt[PATH_MAX + 128];   /* extra room for ANSI escape codes */
    int n = snprintf(prompt, sizeof(prompt),
        CLR_GREEN "myshell" CLR_RESET ":"
        CLR_BLUE  "%s"      CLR_RESET "$ ",
        cwd);
    my_write(STDOUT_FILENO, prompt, (size_t)n);
}

/* ════════════════════════════════════════════════════════════════════
 * MAIN — Read-Parse-Execute (REPL) Loop
 * ════════════════════════════════════════════════════════════════════ */
int main(void)
{
    /* +2: one for '\n' that fgets keeps, one for '\0' */
    char  line[MAX_INPUT + 2];
    cmd_t cmd;

    print_banner();              /* Phase 3: uses my_write + my_getpid  */
    history_init(&g_history);   /* initialise linked-list history       */
    install_signal_handlers();  /* Phase 4: SIGINT + SIGCHLD            */

    while (1) {
        print_prompt();
        fflush(stdout);

        /* fgets reads at most sizeof(line)-1 chars, appends '\0' */
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            my_write(STDOUT_FILENO, "\n", 1);
            break;   /* EOF (Ctrl+D) */
        }

        /* Strip trailing newline */
        line[strcspn(line, "\n")] = '\0';

        /* Skip fully empty input */
        if (line[0] == '\0')
            continue;

        /*
         * Enforce MAX_INPUT (100 char) constraint from proposal.
         * fgets limits the buffer, but if the user typed more than
         * MAX_INPUT chars the buffer holds exactly MAX_INPUT chars
         * and the newline is missing — detect and warn.
         */
        if (strlen(line) > MAX_INPUT) {
            fprintf(stderr,
                "myshell: input too long (max %d characters)\n", MAX_INPUT);
            /* Drain the rest of the oversized line from stdin */
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            continue;
        }

        /* Store in history BEFORE parse_line() — strtok mutates 'line' */
        history_add(&g_history, line);

        /* Phase 1: tokenise into cmd_t */
        if (parse_line(line, &cmd) < 0)
            continue;

        if (cmd.argc == 0)
            continue;

        /* ── Dispatch: built-ins first, then external commands ── */
        if (strcmp(cmd.args[0], "exit") == 0) {
            my_write(STDOUT_FILENO, "Goodbye!\n", 9);
            break;

        } else if (strcmp(cmd.args[0], "cd") == 0) {
            builtin_cd(&cmd);

        } else if (strcmp(cmd.args[0], "pwd") == 0) {
            builtin_pwd();

        } else if (strcmp(cmd.args[0], "history") == 0) {
            history_print(&g_history);

        } else if (strcmp(cmd.args[0], "help") == 0) {
            builtin_help();

        } else {
            /* Phase 2: fork + exec + wait (external command) */
            execute_command(&cmd);
        }
    }

    /* Free all history nodes — zero memory leaks on exit */
    history_free(&g_history);
    return 0;
}
