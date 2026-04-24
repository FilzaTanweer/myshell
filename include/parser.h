/*
 * include/parser.h
 *
 * Lexical Analysis — Phase 1 of the WBS.
 *
 * Constraints from the proposal:
 *   "implementing a parser using strtok() with constraints
 *    (Max 100 chars, 10 arguments)."
 */

#ifndef PARSER_H
#define PARSER_H

#define MAX_INPUT   100   /* maximum input length (proposal constraint)  */
#define MAX_ARGS     10   /* maximum number of arguments (proposal const) */

/* Parsed command result */
typedef struct {
    char *args[MAX_ARGS + 1]; /* NULL-terminated argument array           */
    int   argc;               /* number of actual arguments               */
    int   background;         /* 1 if trailing '&' detected               */
    char *outfile;            /* filename after '>'  (or NULL)            */
    char *infile;             /* filename after '<'  (or NULL)            */
} cmd_t;

/*
 * parse_line() — tokenize 'line' into a cmd_t.
 *
 * 'line' is modified in-place (strtok overwrites delimiters).
 * Returns 0 on success, -1 if input exceeds MAX_ARGS or is malformed.
 */
int parse_line(char *line, cmd_t *cmd);

#endif /* PARSER_H */
