/**
 * @file    utl_ringbuf.h
 * @brief   Lock-free ring buffer — fixed size, SPSC (Single Producer Single Consumer) safe
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Use cases:
 *   - Log buffer writes
 *   - Audio/video data streams
 *   - Data passing between interrupt and main loop
 *   - Fixed-memory embedded system scenarios
 */

#ifndef UTL_RINGBUF_H
#define UTL_RINGBUF_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

typedef struct utl_ringbuf utl_ringbuf_t;

/*===========================================================================
 * API
 *===========================================================================*/

/// @brief Create a ring buffer (capacity as power of 2 improves performance)
utl_err_t utl_ringbuf_create(size_t capacity, utl_ringbuf_t **rb);

void      utl_ringbuf_destroy(utl_ringbuf_t *rb);

/// @brief Write data (best-effort, returns actual bytes written)
utl_err_t utl_ringbuf_write(utl_ringbuf_t *rb, const void *data,
                            size_t size, size_t *written);

/// @brief Read data (best-effort, returns actual bytes read)
utl_err_t utl_ringbuf_read(utl_ringbuf_t *rb, void *buf,
                           size_t size, size_t *read);

/// @brief Peek data without consuming
utl_err_t utl_ringbuf_peek(const utl_ringbuf_t *rb, void *buf,
                           size_t size, size_t *read);

/// @brief Used space
size_t utl_ringbuf_used(const utl_ringbuf_t *rb);

/// @brief Remaining free space
size_t utl_ringbuf_free(const utl_ringbuf_t *rb);

/// @brief Total capacity
size_t utl_ringbuf_capacity(const utl_ringbuf_t *rb);

/// @brief Reset to empty
void   utl_ringbuf_reset(utl_ringbuf_t *rb);

/// @brief Check if empty
bool   utl_ringbuf_is_empty(const utl_ringbuf_t *rb);

/// @brief Check if full
bool   utl_ringbuf_is_full(const utl_ringbuf_t *rb);

UTL_EXTERN_C_END

#endif /* UTL_RINGBUF_H */
