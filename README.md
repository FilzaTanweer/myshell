# Custom Command Line Shell

**OS Project | FAST-NUCES Karachi**
**Instructor:** Minhal Raza
**Authors:** Filza Tanweer (24k-0708), Fatima Hiader (24k-0551), Bismah Sheikh (24k-0795)

## Features
- Process execution via fork() + execvp() + waitpid()
- Low-level syscall wrappers: my_fork, my_write, my_getpid
- Built-in commands: cd, pwd, history, exit
- I/O Redirection (> and <)
- Background execution (&)
- Signal handling (Ctrl+C)
- Command history using linked list (struct history_node)

## Build & Run
```bash
make
./myshell
```

## Run Tests
```bash
bash tests/run_tests.sh
```

## Project Structure
src/        - Source files (myshell.c, parser.c, history.c)
include/    - Headers (my_syscalls.h, history.h, parser.h)
tests/      - Automated test suite
Makefile    - Build system
