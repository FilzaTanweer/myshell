/*
 * src/parser.c
 *
 * Phase 1 — Lexical Analysis (WBS Phase 1, Week 7-8).
 *
 * Tokenises a raw input line into a cmd_t struct.
 * Enforces proposal constraints: max 100 chars input, max 10 arguments.
 *
 * Supported syntax:
 *   cmd [args...]
 *   cmd [args...] > outfile       (output redirect, truncate)
 *   cmd [args...] >> outfile      (output redirect, append)
 *   cmd [args...] < infile        (input redirect)
 *   cmd [args...] &               (background execution)
 *   combinations of the above
 */

#include <stdio.h>
#include <string.h>
#include "parser.h"

int parse_line(char *line, cmd_t *cmd)
{
    /* Zero-initialise the output struct */
    cmd->argc       = 0;
    cmd->background = 0;
    cmd->append     = 0;
    cmd->outfile    = NULL;
    cmd->infile     = NULL;
    for (int i = 0; i <= MAX_ARGS; i++)
        cmd->args[i] = NULL;

    char *token = strtok(line, " \t\n");

    while (token != NULL) {

        if (strcmp(token, "&") == 0) {
            cmd->background = 1;
            /* '&' should be the last meaningful token; stop parsing */
            break;

        } else if (strcmp(token, ">>") == 0) {
            /* Append output redirection */
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '>>'\n");
                return -1;
            }
            /* Reject accidental "&" parsed as filename */
            if (strcmp(token, "&") == 0) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '>>'\n");
                return -1;
            }
            cmd->outfile = token;
            cmd->append  = 1;

        } else if (strcmp(token, ">") == 0) {
            /* Truncating output redirection */
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '>'\n");
                return -1;
            }
            if (strcmp(token, "&") == 0) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '>'\n");
                return -1;
            }
            cmd->outfile = token;
            cmd->append  = 0;

        } else if (strcmp(token, "<") == 0) {
            /* Input redirection */
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '<'\n");
                return -1;
            }
            if (strcmp(token, "&") == 0) {
                fprintf(stderr,
                    "myshell: syntax error: expected filename after '<'\n");
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
