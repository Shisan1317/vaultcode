/**
 * @file    utl_queue.h
 * @brief   General-purpose queue — linked-list-based FIFO queue, object-oriented encapsulation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifndef UTL_QUEUE_H
#define UTL_QUEUE_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

typedef struct utl_queue utl_queue_t;

typedef struct {
    bool     thread_safe;
    uint32_t max_size;
} utl_queue_cfg_t;

/* ---- Lifecycle ---- */
utl_err_t utl_queue_create(const utl_queue_cfg_t *cfg, utl_queue_t **queue);
void      utl_queue_destroy(utl_queue_t *queue);

/* ---- Enqueue / Dequeue ---- */
utl_err_t utl_queue_enqueue(utl_queue_t *queue, void *data);
utl_err_t utl_queue_dequeue(utl_queue_t *queue, void **data);

/* ---- Peek at front (does not remove) ---- */
utl_err_t utl_queue_peek(const utl_queue_t *queue, void **data);

/* ---- Status ---- */
utl_err_t utl_queue_size(const utl_queue_t *queue, size_t *size);
utl_err_t utl_queue_clear(utl_queue_t *queue);
bool      utl_queue_is_empty(const utl_queue_t *queue);

UTL_EXTERN_C_END

#endif /* UTL_QUEUE_H */
