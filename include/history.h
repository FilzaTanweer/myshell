/*
 * include/history.h
 *
 * Command history using a singly-linked list as required by the proposal:
 *   "struct history_node: A linked list or circular buffer for command history."
 *
 * Design: a linked list with a tail pointer for O(1) append.
 * Entries are capped at HISTORY_MAX to avoid unbounded memory growth.
 */

#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX   50    /* maximum entries kept */
#define MAX_CMD_LEN  100    /* proposal: "Max 100 chars" */

/* ── Node definition ─────────────────────────────────────────────── */
typedef struct history_node {
    int                  index;   /* 1-based command number         */
    char                 cmd[MAX_CMD_LEN + 1];
    struct history_node *next;
} history_node_t;

/* ── List handle ──────────────────────────────────────────────────── */
typedef struct {
    history_node_t *head;
    history_node_t *tail;
    int             count;   /* total entries currently in list */
    int             serial;  /* monotonically increasing index  */
} history_list_t;

/* ── API ──────────────────────────────────────────────────────────── */
void history_init  (history_list_t *hl);
void history_add   (history_list_t *hl, const char *cmd);
void history_print (const history_list_t *hl);
void history_free  (history_list_t *hl);

#endif /* HISTORY_H */
