# ══════════════════════════════════════════════════════════════
# Makefile — myshell v2
# FAST-NUCES OS Project | Instructor: Minhal Raza
# Authors: Filza Tanweer, Fatima Haider, Bismah Sheikh
# ══════════════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -g \
           -I./include                 \
           -std=c11

TARGET  = myshell
SRCDIR  = src
INCDIR  = include
TESTDIR = tests

SRCS    = $(SRCDIR)/myshell.c  \
           $(SRCDIR)/parser.c  \
           $(SRCDIR)/history.c

OBJS    = $(SRCS:.c=.o)

# ── Default target ──────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo ""
	@echo "  ✔  Build successful → ./$(TARGET)"
	@echo ""

# Compile each .c → .o
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Run ─────────────────────────────────────────────────────
run: $(TARGET)
	./$(TARGET)

# ── Automated test suite ────────────────────────────────────
test: $(TARGET)
	@echo "═══ Running automated test suite ═══"
	@bash $(TESTDIR)/run_tests.sh

# ── Valgrind memory check ───────────────────────────────────
memcheck: $(TARGET)
	valgrind --leak-check=full --track-origins=yes ./$(TARGET)

# ── Clean build artefacts ───────────────────────────────────
clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
	@echo "  ✔  Cleaned build artefacts."

.PHONY: all run test memcheck clean
