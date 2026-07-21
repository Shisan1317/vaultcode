/**
 * @file    utl_ringbuf.c
 * @brief   Ring buffer — fixed size, SPSC safe
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "utl_ringbuf.h"

struct utl_ringbuf {
    utl_byte_t *buf;
    size_t      capacity;
    size_t      read_idx;    /* volatile for lock-free reads */
    size_t      write_idx;   /* volatile for lock-free writes */
};

utl_err_t utl_ringbuf_create(size_t capacity, utl_ringbuf_t **rb)
{
    if (!rb || capacity == 0) return UTL_ERR_NULL;

    utl_ringbuf_t *r = (utl_ringbuf_t *)malloc(sizeof(utl_ringbuf_t));
    if (!r) return UTL_ERR_MEM;

    /* capacity+1 to distinguish empty/full */
    r->buf = (utl_byte_t *)malloc(capacity + 1);
    if (!r->buf) { free(r); return UTL_ERR_MEM; }

    r->capacity  = capacity;
    r->read_idx  = 0;
    r->write_idx = 0;

    *rb = r;
    return UTL_OK;
}

void utl_ringbuf_destroy(utl_ringbuf_t *rb)
{
    if (!rb) return;
    free(rb->buf);
    free(rb);
}

size_t utl_ringbuf_used(const utl_ringbuf_t *rb)
{
    if (!rb) return 0;
    size_t w = rb->write_idx;
    size_t r = rb->read_idx;
    if (w >= r) return w - r;
    return rb->capacity + 1 + w - r;
}

size_t utl_ringbuf_free(const utl_ringbuf_t *rb)
{
    if (!rb) return 0;
    return rb->capacity - utl_ringbuf_used(rb);
}

size_t utl_ringbuf_capacity(const utl_ringbuf_t *rb) { return rb ? rb->capacity : 0; }
bool   utl_ringbuf_is_empty(const utl_ringbuf_t *rb)  { return utl_ringbuf_used(rb) == 0; }
bool   utl_ringbuf_is_full(const utl_ringbuf_t *rb)   { return utl_ringbuf_free(rb) == 0; }

void utl_ringbuf_reset(utl_ringbuf_t *rb)
{
    if (rb) { rb->read_idx = 0; rb->write_idx = 0; }
}

utl_err_t utl_ringbuf_write(utl_ringbuf_t *rb, const void *data,
                            size_t size, size_t *written)
{
    if (!rb || !data) return UTL_ERR_NULL;
    size_t avail = utl_ringbuf_free(rb);
    if (avail == 0) { if (written) *written = 0; return UTL_ERR_FULL; }

    size_t n = size < avail ? size : avail;
    size_t w = rb->write_idx;
    size_t buf_size = rb->capacity + 1;

    size_t first_chunk = buf_size - w;
    if (first_chunk > n) first_chunk = n;

    memcpy(rb->buf + w, data, first_chunk);
    memcpy(rb->buf, (const utl_byte_t *)data + first_chunk, n - first_chunk);

    rb->write_idx = (w + n) % buf_size;
    if (written) *written = n;
    return (n == size) ? UTL_OK : UTL_ERR_FULL;
}

utl_err_t utl_ringbuf_read(utl_ringbuf_t *rb, void *buf,
                           size_t size, size_t *read)
{
    if (!rb || !buf) return UTL_ERR_NULL;
    size_t used = utl_ringbuf_used(rb);
    if (used == 0) { if (read) *read = 0; return UTL_ERR_EMPTY; }

    size_t n = size < used ? size : used;
    size_t r = rb->read_idx;
    size_t buf_size = rb->capacity + 1;

    size_t first_chunk = buf_size - r;
    if (first_chunk > n) first_chunk = n;

    memcpy(buf, rb->buf + r, first_chunk);
    memcpy((utl_byte_t *)buf + first_chunk, rb->buf, n - first_chunk);

    rb->read_idx = (r + n) % buf_size;
    if (read) *read = n;
    return (n == size) ? UTL_OK : UTL_ERR_EMPTY;
}

utl_err_t utl_ringbuf_peek(const utl_ringbuf_t *rb, void *buf,
                           size_t size, size_t *read)
{
    if (!rb || !buf) return UTL_ERR_NULL;
    size_t used = utl_ringbuf_used(rb);
    if (used == 0) { if (read) *read = 0; return UTL_ERR_EMPTY; }

    size_t n = size < used ? size : used;
    size_t r = rb->read_idx;
    size_t buf_size = rb->capacity + 1;

    size_t first_chunk = buf_size - r;
    if (first_chunk > n) first_chunk = n;

    memcpy(buf, rb->buf + r, first_chunk);
    memcpy((utl_byte_t *)buf + first_chunk, rb->buf, n - first_chunk);

    if (read) *read = n;
    return UTL_OK;
}
