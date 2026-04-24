/*
 * src/myshell.c  —  Custom Command Line Shell
 *
 * FAST-NUCES | Operating Systems Project
 * Instructor : Minhal Raza
 * Authors    : Filza Tanweer (24k-0708)
 *              Fatima Hiader (24k-0551)
 *              Bismah Sheikh (24k-0795)
 *
 * ════════════════════════════════════════════════════════════════════
 * WBS PHASE MAPPING
 * ════════════════════════════════════════════════════════════════════
 * Phase 1 — Lexical Analysis         → parser.c / parser.h
 * Phase 2 — Process Control          → execute_command() here
 * Phase 3 — Low-Level Syscalls       → my_syscalls.h (my_fork,
 *                                       my_write, my_getpid)
 * Phase 4 — Features & Redirection   → execute_command() +
 *                                       sigaction/sigprocmask
 * Phase 5 — Testing & Documentation  → tests/ directory
 * ════════════════════════════════════════════════════════════════════
 *
 * KEY OS CONCEPTS DEMONSTRATED
 * ─────────────────────────────
 * • fork()   : duplicates address space (parent-child creation)
 * • execvp() : replaces process image (program execution)
 * • waitpid(): prevents zombie processes (parent-child sync)
 * • sigaction / sigprocmask : signal masks & Ctrl+C handling
 * • dup2()   : file descriptor manipulation for I/O redirection
 * • syscall(): direct kernel interface bypassing glibc
 */

/* Enable POSIX.1-2008 + GNU extensions for sigaction, sigset_t, etc.
 * Must be defined before any system header is included.            */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
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

/* Project headers */
#include "../include/my_syscalls.h"
#include "../include/history.h"
#include "../include/parser.h"

/* ─── Prompt colours (ANSI escape codes) ──────────────────────────── */
#define CLR_GREEN  "\033[1;32m"
#define CLR_BLUE   "\033[1;34m"
#define CLR_RESET  "\033[0m"

/* ─── Global history list ─────────────────────────────────────────── */
static history_list_t g_history;

/* ════════════════════════════════════════════════════════════════════
 * PHASE 3 — LOW-LEVEL SYSCALL DEMO
 * Prints shell banner + PID using my_write() and my_getpid(),
 * both of which call the kernel directly via syscall().
 * ════════════════════════════════════════════════════════════════════ */
static void print_banner(void)
{
    pid_t pid = my_getpid();

    char banner[512];
    int  n = snprintf(banner, sizeof(banner),
        "\n"
        "  myshell -- OS Project (FAST-NUCES | Minhal Raza)\n"
        "  Shell PID (via my_getpid syscall): %d\n"
        "  Type 'help' for commands, 'exit' to quit.\n\n",
        pid);

    my_write(STDOUT_FILENO, banner, (size_t)n);
}

/* ═══════════════════════════════════════════════════════════════════
 * PHASE 4 — SIGNAL HANDLING
 *
 * handle_sigint():  SIGINT handler for the SHELL process.
 *   - Shell ignores Ctrl+C (does not exit).
 *   - Writes a newline via my_write() (async-signal-safe).
 *   - Child processes restore SIG_DFL before exec(), so Ctrl+C
 *     terminates foreground child normally.
 *
 * handle_sigchld(): SIGCHLD handler.
 *   - Reaps any finished background child processes via waitpid()
 *     with WNOHANG, preventing zombie processes.
 * ═══════════════════════════════════════════════════════════════════ */
static void handle_sigint(int sig)
{
    (void)sig;
    my_write(STDOUT_FILENO, "\n", 1);  /* async-signal-safe write */
}

static void handle_sigchld(int sig)
{
    (void)sig;
    int saved_errno = errno;
    /* Reap ALL finished children without blocking */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

static void install_signal_handlers(void)
{
    struct sigaction sa_int, sa_chld;

    /* ── SIGINT: shell ignores, SA_RESTART resumes interrupted calls ── */
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    /* ── SIGCHLD: auto-reap background children (zombie prevention) ── */
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0) {
        perror("sigaction SIGCHLD");
        exit(EXIT_FAILURE);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * PHASE 2 — CORE PROCESS CONTROL
 *
 * execute_command() implements the fork-exec-wait loop:
 *
 *   my_fork()  →  creates child (duplicates address space)
 *   execvp()   →  replaces child's image with requested program
 *   waitpid()  →  parent blocks until foreground child finishes
 *                  (prevents zombie processes for foreground cmds)
 *
 * PHASE 4 additions: I/O redirection + signal mask in child.
 * ═══════════════════════════════════════════════════════════════════ */
static void execute_command(const cmd_t *cmd)
{
    /*
     * PHASE 3: Use my_fork() — direct kernel syscall(SYS_fork)
     * instead of the glibc fork() wrapper.
     */
    pid_t pid = my_fork();

    if (pid < 0) {
        perror("my_fork");
        return;
    }

    /* ── CHILD process ── */
    if (pid == 0) {
        /*
         * Restore default SIGINT disposition in child.
         * This means Ctrl+C will terminate the child (foreground
         * program), but the parent shell (which has our custom
         * handler) will survive.
         *
         * Also restore SIGCHLD default so child's own children
         * work normally.
         */
        signal(SIGINT,  SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /*
         * PHASE 4 — Signal mask:
         * Unblock all signals in the child so programs like
         * sleep, cat, etc. respond to Ctrl+C normally.
         */
        sigset_t mask;
        sigemptyset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        /* ── Input redirection (<) ── */
        if (cmd->infile) {
            int fd = open(cmd->infile, O_RDONLY);
            if (fd < 0) {
                perror("open (input redirection)");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        /* ── Output redirection (>) ── */
        if (cmd->outfile) {
            int fd = open(cmd->outfile,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
            if (fd < 0) {
                perror("open (output redirection)");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        /*
         * execvp() searches $PATH for the command.
         * Works for absolute paths (/bin/ls),
         *           relative paths (./myprog),
         *       and PATH-searched names (ls, echo, ...).
         */
        if (execvp(cmd->args[0], cmd->args) < 0) {
            /* perror() uses the glibc errno string; acceptable here */
            fprintf(stderr,
                "myshell: '%s': %s\n", cmd->args[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /* ── PARENT process ── */
    if (!cmd->background) {
        /*
         * waitpid() — synchronise with the foreground child.
         * This prevents the child from becoming a zombie by
         * collecting its exit status before returning to the prompt.
         */
        int status;
        waitpid(pid, &status, 0);

        if (WIFSIGNALED(status)) {
            /* Child killed by signal (e.g., Ctrl+C → SIGINT) */
            fprintf(stderr, "\n[killed by signal %d]\n", WTERMSIG(status));
        }
    } else {
        /*
         * Background (&): parent does NOT wait.
         * The SIGCHLD handler (handle_sigchld) will reap this
         * child asynchronously when it finishes, preventing zombies.
         */
        char msg[64];
        int  n = snprintf(msg, sizeof(msg),
                     "[%d] started in background\n", pid);
        my_write(STDOUT_FILENO, msg, (size_t)n);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * BUILT-IN COMMANDS  (Phase 3 — "without using system()")
 *
 * cd    → chdir() syscall
 * pwd   → getcwd() syscall
 * exit  → free resources, then exit()
 * history → traverse linked list and print
 * help  → print command reference
 * ═══════════════════════════════════════════════════════════════════ */
static void builtin_cd(const cmd_t *cmd)
{
    const char *target = (cmd->argc > 1) ? cmd->args[1] : getenv("HOME");
    if (target == NULL) target = "/";
    if (chdir(target) != 0)
        perror("cd");
}

static void builtin_pwd(void)
{
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd))) {
        /* Use my_write for consistency with the low-level syscall theme */
        size_t len = strlen(cwd);
        cwd[len]   = '\n';
        my_write(STDOUT_FILENO, cwd, len + 1);
    } else {
        perror("pwd");
    }
}

static void builtin_help(void)
{
    static const char *help =
        "\n  ── myshell built-in commands ──────────────────────\n"
        "  cd [dir]       Change directory (default: HOME)\n"
        "  pwd            Print working directory\n"
        "  history        Show command history (linked list)\n"
        "  exit           Exit the shell\n"
        "  help           Show this help\n\n"
        "  ── execution features ─────────────────────────────\n"
        "  <cmd> &        Run command in background\n"
        "  <cmd> > file   Redirect stdout to file\n"
        "  <cmd> < file   Redirect stdin from file\n"
        "  /abs/path      Absolute path execution\n"
        "  ./rel/path     Relative path execution\n\n"
        "  ── low-level syscall wrappers (Phase 3) ───────────\n"
        "  my_fork()    → syscall(SYS_fork)\n"
        "  my_write()   → syscall(SYS_write)\n"
        "  my_getpid()  → syscall(SYS_getpid)\n"
        "  ───────────────────────────────────────────────────\n\n";
    my_write(STDOUT_FILENO, help, strlen(help));
}

/* ═══════════════════════════════════════════════════════════════════
 * PROMPT HELPER
 * Uses my_write (syscall-based) to print the coloured prompt.
 * ═══════════════════════════════════════════════════════════════════ */
static void print_prompt(void)
{
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        strcpy(cwd, "?");

    char prompt[640];
    int n = snprintf(prompt, sizeof(prompt),
        CLR_GREEN "myshell" CLR_RESET ":"
        CLR_BLUE  "%s"      CLR_RESET "$ ",
        cwd);
    my_write(STDOUT_FILENO, prompt, (size_t)n);
}

/* ═══════════════════════════════════════════════════════════════════
 * MAIN — Read-Parse-Execute loop
 * ═══════════════════════════════════════════════════════════════════ */
int main(void)
{
    char  line[MAX_INPUT + 2];   /* +2: newline + NUL */
    cmd_t cmd;

    /* Phase 3: banner printed via my_write() + my_getpid() */
    print_banner();

    /* Initialise linked-list history */
    history_init(&g_history);

    /* Phase 4: install signal handlers */
    install_signal_handlers();

    /* ── REPL ─────────────────────────────────────────────── */
    while (1) {
        print_prompt();
        fflush(stdout);

        /* Read one line (up to MAX_INPUT chars — proposal constraint) */
        if (fgets(line, sizeof(line), stdin) == NULL) {
            my_write(STDOUT_FILENO, "\n", 1);
            break;   /* EOF (Ctrl+D) */
        }

        /* Strip newline */
        line[strcspn(line, "\n")] = '\0';

        /* Skip blank input */
        if (line[0] == '\0')
            continue;

        /*
         * Enforce MAX_INPUT (100 char) constraint.
         * fgets already limits the buffer, but we warn the user
         * if they somehow sent more than 100 visible chars.
         */
        if (strlen(line) > MAX_INPUT) {
            fprintf(stderr,
                "myshell: input too long (max %d characters)\n", MAX_INPUT);
            /* Flush remaining input from the buffer */
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        /* Add to linked-list history BEFORE parsing (parse mutates line) */
        history_add(&g_history, line);

        /* Phase 1 — Lexical Analysis */
        if (parse_line(line, &cmd) < 0)
            continue;   /* parse error already printed */

        if (cmd.argc == 0)
            continue;   /* empty after stripping redirects */

        /* ── Dispatch built-ins ── */
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
            /* Phase 2 — External command: my_fork + execvp + waitpid */
            execute_command(&cmd);
        }
    }

    history_free(&g_history);
    return 0;
}
