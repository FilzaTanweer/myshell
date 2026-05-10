/*
 * include/history.h
 *
 * Command history using a singly-linked list with tail pointer.
 * Proposal requirement: "struct history_node: A linked list or
 * circular buffer for command history."
 *
 * Design: tail pointer gives O(1) append. When count reaches
 * HISTORY_MAX, the oldest node (head) is evicted — giving
 * circular-buffer eviction semantics on a linked list.
 */

#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX   50              /* maximum entries kept            */
#define MAX_CMD_LEN  100              /* proposal: "Max 100 chars"       */

/* ── Node ─────────────────────────────────────────────────────────── */
typedef struct history_node {
    int                  index;       /* 1-based command number          */
    char                 cmd[MAX_CMD_LEN + 1];
    struct history_node *next;
} history_node_t;

/* ── List handle ──────────────────────────────────────────────────── */
typedef struct {
    history_node_t *head;             /* oldest entry                    */
    history_node_t *tail;             /* most recent entry (O(1) append) */
    int             count;            /* current entries in list         */
    int             serial;           /* monotonically increasing index  */
} history_list_t;

/* ── API ──────────────────────────────────────────────────────────── */
void history_init  (history_list_t *hl);
void history_add   (history_list_t *hl, const char *cmd);
void history_print (const history_list_t *hl);
void history_free  (history_list_t *hl);

#endif /* HISTORY_H */
