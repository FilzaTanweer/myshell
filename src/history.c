/*
 * src/history.c
 *
 * Linked-list command history implementation.
 * See include/history.h for the data structure and API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

/* ── history_init ─────────────────────────────────────────────────── */
void history_init(history_list_t *hl)
{
    hl->head   = NULL;
    hl->tail   = NULL;
    hl->count  = 0;
    hl->serial = 0;
}

/* ── history_add ──────────────────────────────────────────────────── */
/*
 * Appends a new node to the tail of the linked list.
 * If the list is full (HISTORY_MAX entries), the oldest node (head)
 * is removed first — giving circular-buffer semantics on a linked list.
 */
void history_add(history_list_t *hl, const char *cmd)
{
    /* ── Evict oldest if at capacity ── */
    if (hl->count == HISTORY_MAX) {
        history_node_t *old = hl->head;
        hl->head = old->next;
        if (hl->head == NULL)
            hl->tail = NULL;
        free(old);
        hl->count--;
    }

    /* ── Allocate and populate new node ── */
    history_node_t *node = malloc(sizeof(history_node_t));
    if (!node) {
        perror("history malloc");
        return;
    }
    node->index = ++(hl->serial);
    strncpy(node->cmd, cmd, MAX_CMD_LEN);
    node->cmd[MAX_CMD_LEN] = '\0';
    node->next = NULL;

    /* ── Append to tail ── */
    if (hl->tail == NULL) {
        hl->head = hl->tail = node;
    } else {
        hl->tail->next = node;
        hl->tail = node;
    }
    hl->count++;
}

/* ── history_print ────────────────────────────────────────────────── */
void history_print(const history_list_t *hl)
{
    if (hl->count == 0) {
        printf("  (no history)\n");
        return;
    }
    history_node_t *cur = hl->head;
    while (cur) {
        printf("  %3d  %s\n", cur->index, cur->cmd);
        cur = cur->next;
    }
}

/* ── history_free ─────────────────────────────────────────────────── */
void history_free(history_list_t *hl)
{
    history_node_t *cur = hl->head;
    while (cur) {
        history_node_t *next = cur->next;
        free(cur);
        cur = next;
    }
    hl->head  = NULL;
    hl->tail  = NULL;
    hl->count = 0;
}
