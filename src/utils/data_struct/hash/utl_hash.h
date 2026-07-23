/**
 * @file    utl_hash.h
 * @brief   General-purpose hash table module — object-oriented encapsulation, chaining method for collision resolution
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Features:
 *   - OOP encapsulation, struct forward-declared, internal members fully private
 *   - Chaining method (separate chaining) for hash collision resolution
 *   - Configurable at creation: bucket_count, thread_safe, max_size
 *   - Unified utl_err_t error code return
 *   - Both integer key and string key modes
 *   - Supports iteration to get all keys
 */

#ifndef UTL_HASH_H
#define UTL_HASH_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Type forward declarations
 *===========================================================================*/

typedef struct utl_hash utl_hash_t;

/// @brief Hash iterator (for traversal)
typedef struct {
    void   *key;       ///< Current key
    void   *value;     ///< Current value
    /* Internal iteration state (private) */
    size_t  _bucket;
    void   *_node;
} utl_hash_iter_t;

/*===========================================================================
 * Key type & configuration
 *===========================================================================*/

typedef enum {
    UTL_HASH_KEY_INT  = 0,   ///< Integer key (uint64_t)
    UTL_HASH_KEY_STR  = 1,   ///< String key (const char*)
} utl_hash_key_type_t;

typedef struct {
    uint32_t          bucket_count;  ///< Bucket count (recommended primes: 101, 503, 1009), 0 = use default
    utl_hash_key_type_t key_type;    ///< Key type
    bool              thread_safe;   ///< Thread-safe switch
    uint32_t          max_size;      ///< Maximum node count, 0 = unlimited
} utl_hash_cfg_t;

/*===========================================================================
 * API — Lifecycle
 *===========================================================================*/

utl_err_t utl_hash_create(const utl_hash_cfg_t *cfg, utl_hash_t **hash);
void      utl_hash_destroy(utl_hash_t *hash);

/*===========================================================================
 * API — Insert, delete, query
 *===========================================================================*/

utl_err_t utl_hash_put(utl_hash_t *hash, void *key, void *value);
utl_err_t utl_hash_get(const utl_hash_t *hash, const void *key, void **value);
utl_err_t utl_hash_remove(utl_hash_t *hash, const void *key);
utl_err_t utl_hash_contains(const utl_hash_t *hash, void *key, bool *found);
utl_err_t utl_hash_clear(utl_hash_t *hash);
utl_err_t utl_hash_size(const utl_hash_t *hash, size_t *size);

/*===========================================================================
 * API — Traversal
 *===========================================================================*/

/// @brief Initialize iterator pointing to the first element
utl_err_t utl_hash_iter_begin(const utl_hash_t *hash, utl_hash_iter_t *iter);

/// @brief Move to the next element
utl_err_t utl_hash_iter_next(const utl_hash_t *hash, utl_hash_iter_t *iter);

/// @brief Whether the iterator has reached the end
bool utl_hash_iter_end(const utl_hash_iter_t *iter);

UTL_EXTERN_C_END

#endif /* UTL_HASH_H */
