# ══════════════════════════════════════════════════════
# Makefile — myshell v2
# FAST-NUCES OS Project | Instructor: Minhal Raza
# ══════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -g \
          -I./include              \
          -std=c11

TARGET  = myshell
SRCDIR  = src
INCDIR  = include
TESTDIR = tests

SRCS    = $(SRCDIR)/myshell.c   \
          $(SRCDIR)/parser.c    \
          $(SRCDIR)/history.c

OBJS    = $(SRCS:.c=.o)

# ── Default target ──────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo ""
	@echo "  ✔  Build successful → ./$(TARGET)"
	@echo ""

# Pattern rule: compile each .c → .o
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Run ─────────────────────────────────────────────
run: $(TARGET)
	./$(TARGET)

# ── Tests ───────────────────────────────────────────
test: $(TARGET)
	@echo "═══ Running test suite ═══"
	@bash $(TESTDIR)/run_tests.sh

# ── Clean ───────────────────────────────────────────
clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
	@echo "  ✔  Cleaned build artefacts."

.PHONY: all run test clean
