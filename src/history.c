/*
 * src/history.c
 *
 * Linked-list command history implementation.
 * See include/history.h for data structure and API.
 *
 * Eviction policy: when count reaches HISTORY_MAX (50), the head
 * node (oldest command) is freed and removed before the new node
 * is appended to the tail. This gives O(1) append and O(1) eviction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

void history_init(history_list_t *hl)
{
    hl->head   = NULL;
    hl->tail   = NULL;
    hl->count  = 0;
    hl->serial = 0;
}

/*
 * history_add() — append a new command to the tail.
 * If list is full, evict the oldest (head) node first.
 */
void history_add(history_list_t *hl, const char *cmd)
{
    /* Evict oldest entry if at capacity */
    if (hl->count == HISTORY_MAX) {
        history_node_t *old = hl->head;
        hl->head = old->next;
        if (hl->head == NULL)
            hl->tail = NULL;
        free(old);
        hl->count--;
    }

    history_node_t *node = malloc(sizeof(history_node_t));
    if (!node) {
        perror("history malloc");
        return;
    }
    node->index = ++(hl->serial);
    strncpy(node->cmd, cmd, MAX_CMD_LEN);
    node->cmd[MAX_CMD_LEN] = '\0';
    node->next = NULL;

    if (hl->tail == NULL) {
        hl->head = hl->tail = node;
    } else {
        hl->tail->next = node;
        hl->tail = node;
    }
    hl->count++;
}

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

/*
 * history_free() — release all heap-allocated nodes.
 * Called on shell exit to ensure zero memory leaks.
 */
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
