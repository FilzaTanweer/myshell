/*
 * src/parser.c
 *
 * Phase 1 — Lexical Analysis (WBS Phase 1, Week 7-8).
 *
 * Tokenizes a raw input line into a cmd_t struct, enforcing the
 * proposal constraints: max 100 chars input, max 10 arguments.
 */

#include <stdio.h>
#include <string.h>
#include "parser.h"

/* ── parse_line ───────────────────────────────────────────────────── */
int parse_line(char *line, cmd_t *cmd)
{
    /* Zero-initialise the output struct */
    cmd->argc       = 0;
    cmd->background = 0;
    cmd->outfile    = NULL;
    cmd->infile     = NULL;
    for (int i = 0; i <= MAX_ARGS; i++)
        cmd->args[i] = NULL;

    char *token = strtok(line, " \t\n");

    while (token != NULL) {
        if (strcmp(token, "&") == 0) {
            /* Background flag — must be last token */
            cmd->background = 1;

        } else if (strcmp(token, ">") == 0) {
            /* Output redirection: next token is the filename */
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "myshell: syntax error: expected filename after '>'\n");
                return -1;
            }
            cmd->outfile = token;

        } else if (strcmp(token, "<") == 0) {
            /* Input redirection: next token is the filename */
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "myshell: syntax error: expected filename after '<'\n");
                return -1;
            }
            cmd->infile = token;

        } else {
            /* Regular argument — enforce MAX_ARGS constraint */
            if (cmd->argc >= MAX_ARGS) {
                fprintf(stderr,
                    "myshell: too many arguments (max %d allowed)\n", MAX_ARGS);
                return -1;
            }
            cmd->args[cmd->argc++] = token;
        }

        token = strtok(NULL, " \t\n");
    }

    cmd->args[cmd->argc] = NULL;   /* NULL-terminate for execvp() */
    return 0;
}
