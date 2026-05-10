# myshell — Custom Unix Command Line Shell

**FAST-NUCES | Operating Systems Project**
**Instructor:** Minhal Raza
**Authors:** Filza Tanweer (24k-0708) · Fatima Haider (24k-0551) · Bismah Sheikh (24k-0795)

---

## Overview

myshell is a lightweight, interactive Unix-like shell built from scratch in C for Ubuntu Linux. It demonstrates core Operating Systems concepts — process creation, execution, synchronisation, signal handling, and I/O redirection — using low-level mechanisms at every step.

## Project Structure

```
myshell_v2/
├── Makefile
├── README.md
├── src/
│   ├── myshell.c      Main REPL, signal handlers, built-ins, execute_command()
│   ├── parser.c       Lexical analyser — tokenisation and constraint enforcement
│   └── history.c      Singly-linked list for command history
├── include/
│   ├── my_syscalls.h  Direct kernel syscall wrappers (my_fork, my_write, my_getpid)
│   ├── parser.h       cmd_t struct, MAX_INPUT=100, MAX_ARGS=10
│   └── history.h      history_node_t struct, history_list_t API
└── tests/
    └── run_tests.sh   Automated test suite (23 tests)
```

## Build & Run

```bash
# Build
make

# Run the shell
./myshell

# Run automated tests
make test

# Clean build artefacts
make clean
```

**Requirements:** gcc, Ubuntu 22.04/24.04 LTS, bash (for tests), python3 (for long-input test), valgrind (optional)

## Features

| Feature | Implementation |
|---|---|
| Lexical analysis | `strtok()`-based parser, max 100 chars, max 10 args |
| Process control | `my_fork()` + `execvp()` + `waitpid()` |
| Built-in commands | `cd`, `pwd`, `exit`, `history`, `help` (no `system()`) |
| Output redirection | `>` (truncate) and `>>` (append) via `dup2()` |
| Input redirection | `<` via `dup2()` |
| Background execution | `&` operator with SIGCHLD-based zombie prevention |
| Signal handling | `SIGINT` (Ctrl+C) via `sigaction()` + `SA_RESTART` |
| Command history | Singly-linked list, max 50 entries, O(1) append |
| Direct syscalls | `my_fork()`, `my_write()`, `my_getpid()` via `syscall()` |
| Coloured prompt | ANSI colours via `my_write()` |

## OS Concepts Demonstrated

- **fork()** — address space duplication (parent-child creation)
- **execvp()** — process image replacement (program loading)
- **waitpid()** — parent-child synchronisation, zombie prevention
- **sigaction()** — signal interception and custom handlers
- **sigprocmask()** — signal mask management in child processes
- **dup2()** — file descriptor manipulation for I/O redirection
- **syscall()** — direct kernel interface bypassing glibc
- **chdir()** — why `cd` must be a shell built-in


