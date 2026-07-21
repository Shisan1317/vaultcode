/**
 * @file    utl_queue.c
 * @brief   Queue — linked-list-based FIFO implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "utl_queue.h"
#include "os_mutex.h"

typedef struct queue_node {
    void             *data;
    struct queue_node *next;
} queue_node_t;

struct utl_queue {
    queue_node_t *head;
    queue_node_t *tail;
    size_t        size;
    uint32_t      max_size;
    bool          thread_safe;
    os_mutex_t   *mutex;
};

static const utl_queue_cfg_t kDefaultCfg = {
    .thread_safe = false,
    .max_size    = 0,
};

static inline int prv_lock(utl_queue_t *q) {
    if (q->thread_safe && q->mutex) return os_mutex_lock(q->mutex);
    return 0;
}
static inline int prv_unlock(utl_queue_t *q) {
    if (q->thread_safe && q->mutex) return os_mutex_unlock(q->mutex);
    return 0;
}

/* ---- API ---- */

utl_err_t utl_queue_create(const utl_queue_cfg_t *cfg, utl_queue_t **queue)
{
    if (!queue) return UTL_ERR_NULL;
    const utl_queue_cfg_t *c = cfg ? cfg : &kDefaultCfg;

    utl_queue_t *q = (utl_queue_t *)malloc(sizeof(utl_queue_t));
    if (!q) return UTL_ERR_MEM;

    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->max_size    = c->max_size;
    q->thread_safe = c->thread_safe;
    q->mutex       = NULL;

    if (q->thread_safe) {
        utl_err_t ret = os_mutex_create(&q->mutex);
        if (UTL_FAIL(ret)) { free(q); return ret; }
    }

    *queue = q;
    return UTL_OK;
}

void utl_queue_destroy(utl_queue_t *queue)
{
    if (!queue) return;
    utl_queue_clear(queue);
    if (queue->thread_safe && queue->mutex) os_mutex_destroy(queue->mutex);
    free(queue);
}

utl_err_t utl_queue_enqueue(utl_queue_t *queue, void *data)
{
    if (!queue || !data) return UTL_ERR_NULL;
    prv_lock(queue);

    if (queue->max_size > 0 && queue->size >= queue->max_size) {
        prv_unlock(queue);
        return UTL_ERR_FULL;
    }

    queue_node_t *node = (queue_node_t *)malloc(sizeof(queue_node_t));
    if (!node) { prv_unlock(queue); return UTL_ERR_MEM; }

    node->data = data;
    node->next = NULL;

    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->size++;

    prv_unlock(queue);
    return UTL_OK;
}

utl_err_t utl_queue_dequeue(utl_queue_t *queue, void **data)
{
    if (!queue || !data) return UTL_ERR_NULL;
    prv_lock(queue);

    if (!queue->head) { prv_unlock(queue); return UTL_ERR_EMPTY; }

    queue_node_t *node = queue->head;
    *data = node->data;
    queue->head = node->next;
    if (!queue->head) queue->tail = NULL;
    queue->size--;

    prv_unlock(queue);
    free(node);
    return UTL_OK;
}

utl_err_t utl_queue_peek(const utl_queue_t *queue, void **data)
{
    if (!queue || !data) return UTL_ERR_NULL;
    prv_lock((utl_queue_t *)queue);

    if (!queue->head) { prv_unlock((utl_queue_t *)queue); return UTL_ERR_EMPTY; }
    *data = queue->head->data;

    prv_unlock((utl_queue_t *)queue);
    return UTL_OK;
}

utl_err_t utl_queue_size(const utl_queue_t *queue, size_t *size)
{
    if (!queue || !size) return UTL_ERR_NULL;
    prv_lock((utl_queue_t *)queue);
    *size = queue->size;
    prv_unlock((utl_queue_t *)queue);
    return UTL_OK;
}

utl_err_t utl_queue_clear(utl_queue_t *queue)
{
    if (!queue) return UTL_ERR_NULL;
    prv_lock(queue);

    queue_node_t *cur = queue->head;
    while (cur) {
        queue_node_t *next = cur->next;
        free(cur);
        cur = next;
    }
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    prv_unlock(queue);
    return UTL_OK;
}

bool utl_queue_is_empty(const utl_queue_t *queue)
{
    if (!queue) return true;
    return (queue->head == NULL);
}
