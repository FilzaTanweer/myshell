#!/usr/bin/env bash
# ======================================================
# tests/run_tests.sh  —  Automated test suite for myshell v2
# ======================================================

SHELL_BIN="./myshell"
PASS=0; FAIL=0
RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'; RST='\033[0m'

run_test() {
    local desc="$1" cmds="$2" expect="$3" output
    output=$(printf '%b' "$cmds" | "$SHELL_BIN" 2>&1)
    if echo "$output" | grep -qF "$expect"; then
        echo -e "  ${GRN}PASS${RST}  $desc"; ((PASS++))
    else
        echo -e "  ${RED}FAIL${RST}  $desc"
        echo    "         Expected : '$expect'"
        echo    "         Got      : $(echo "$output" | tail -3)"
        ((FAIL++))
    fi
}

echo ""
echo "======================================================"
echo "  myshell v2 -- Test Suite"
echo "======================================================"

# Phase 3: Low-level syscall wrappers
echo -e "\n${YLW}Phase 3 -- Low-level syscall wrappers${RST}"
run_test "Banner prints Shell PID via my_getpid()" \
    "exit\n" "my_getpid"

# Phase 1: Parsing
echo -e "\n${YLW}Phase 1 -- Lexical Analysis / Parsing${RST}"
run_test "Empty line does not crash" \
    "   \nexit\n" "Goodbye"
run_test "Single command (echo hello_world)" \
    "echo hello_world\nexit\n" "hello_world"
run_test "Multi-arg command (echo alpha beta gamma)" \
    "echo alpha beta gamma\nexit\n" "alpha beta gamma"
run_test "Too many arguments (>10) gives error" \
    "echo a b c d e f g h i j k\nexit\n" "too many arguments"
run_test "Syntax error: missing file after >" \
    "echo test >\nexit\n" "syntax error"

# Phase 2: Process Control
echo -e "\n${YLW}Phase 2 -- fork/exec/wait (Process Control)${RST}"
run_test "ls executes via fork+execvp" \
    "ls\nexit\n" "myshell"
run_test "Absolute path /bin/echo works" \
    "/bin/echo abs_path_ok\nexit\n" "abs_path_ok"
run_test "Relative path ls . shows myshell" \
    "ls .\nexit\n" "myshell"
run_test "Invalid command: error not crash" \
    "zzz_fake_cmd\nexit\n" "zzz_fake_cmd"

# Phase 3: Built-in Commands
echo -e "\n${YLW}Phase 3 -- Built-in Commands (cd, pwd, exit)${RST}"
run_test "pwd prints directory" \
    "pwd\nexit\n" "/"
run_test "cd /tmp then pwd shows /tmp" \
    "cd /tmp\npwd\nexit\n" "/tmp"
run_test "cd no arg goes HOME" \
    "cd\npwd\nexit\n" "$HOME"
run_test "exit prints Goodbye" \
    "exit\n" "Goodbye"
run_test "help lists built-ins" \
    "help\nexit\n" "cd"

# Phase 3: Command History
echo -e "\n${YLW}Phase 3 -- Command History (linked-list)${RST}"
run_test "history stores and shows previous commands" \
    "echo cmd_alpha\nhistory\nexit\n" "cmd_alpha"
run_test "history uses 1-based numbering" \
    "echo first\nhistory\nexit\n" "1"
run_test "history preserves order" \
    "echo aaa\necho bbb\nhistory\nexit\n" "bbb"

# Phase 4: I/O Redirection
echo -e "\n${YLW}Phase 4 -- I/O Redirection${RST}"

TMPOUT=$(mktemp)
printf 'echo redir_output > %s\nexit\n' "$TMPOUT" | "$SHELL_BIN" >/dev/null 2>&1
if grep -q "redir_output" "$TMPOUT" 2>/dev/null; then
    echo -e "  ${GRN}PASS${RST}  Output redirection (>) works"; ((PASS++))
else
    echo -e "  ${RED}FAIL${RST}  Output redirection (>) failed"; ((FAIL++))
fi
rm -f "$TMPOUT"

TMPIN=$(mktemp); echo "from_file_content" > "$TMPIN"
out=$(printf 'cat < %s\nexit\n' "$TMPIN" | "$SHELL_BIN" 2>&1)
if echo "$out" | grep -q "from_file_content"; then
    echo -e "  ${GRN}PASS${RST}  Input redirection (<) works"; ((PASS++))
else
    echo -e "  ${RED}FAIL${RST}  Input redirection (<) failed"; ((FAIL++))
fi
rm -f "$TMPIN"

# Phase 4: Background Execution
echo -e "\n${YLW}Phase 4 -- Background Execution (&)${RST}"
run_test "Background job starts and prints PID" \
    "sleep 2 &\nexit\n" "background"

# Phase 4: 100-char input constraint
echo -e "\n${YLW}Phase 4 -- Parser Constraint (100 char limit)${RST}"
LONG_CMD=$(python3 -c "print('echo ' + 'a'*100)")
run_test "Input > 100 chars triggers length warning" \
    "${LONG_CMD}\nexit\n" "too long"

# Phase 4: Signal Handling note
echo -e "\n${YLW}Phase 4 -- Signal Handling (Ctrl+C)${RST}"
echo -e "  ${YLW}MANUAL${RST}  Run ./myshell, type 'sleep 10', press Ctrl+C."
echo    "          Shell must survive and show prompt again."

# Summary
echo ""
echo "======================================================"
TOTAL=$((PASS+FAIL))
echo -e "  Results: ${GRN}${PASS} passed${RST}, ${RED}${FAIL} failed${RST} / ${TOTAL} total"
echo "======================================================"
echo ""
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
