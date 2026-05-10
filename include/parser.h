/*
 * include/parser.h
 *
 * Lexical Analysis — Phase 1 of the WBS.
 *
 * Proposal constraints:
 *   "implementing a parser using strtok() with constraints
 *    (Max 100 chars, 10 arguments)."
 */

#ifndef PARSER_H
#define PARSER_H

/* Undefine system MAX_INPUT (linux/limits.h type-ahead buffer = 255)
 * before defining our proposal-specified shell input limit of 100. */
#ifdef MAX_INPUT
#undef MAX_INPUT
#endif
#define MAX_INPUT   100   /* maximum input line length (proposal constraint) */
#define MAX_ARGS     10   /* maximum number of arguments (proposal constraint) */

/*
 * cmd_t — parsed command result.
 * Produced by parse_line() and consumed by execute_command() and built-ins.
 * args[] is NULL-terminated so it can be passed directly to execvp().
 */
typedef struct {
    char *args[MAX_ARGS + 1]; /* NULL-terminated argument array         */
    int   argc;               /* number of actual arguments             */
    int   background;         /* 1 if trailing '&' detected             */
    char *outfile;            /* filename after '>'  (or NULL)          */
    char *infile;             /* filename after '<'  (or NULL)          */
    int   append;             /* 1 if '>>' (append mode), 0 if '>'      */
} cmd_t;

/*
 * parse_line() — tokenise 'line' into a cmd_t.
 * 'line' is modified in-place (strtok overwrites delimiters).
 * Returns 0 on success, -1 on error (error message already printed).
 */
int parse_line(char *line, cmd_t *cmd);

#endif /* PARSER_H */
