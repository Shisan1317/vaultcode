/**
 * @file    utl_storage.h
 * @brief   Storage layer — file-based key-value persistence, optional encryption
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides simple key-value file storage
 *   - Optional AES encryption (via crypto module)
 *   - Depends on platform layer file interface
 */

#ifndef UTL_STORAGE_H
#define UTL_STORAGE_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

typedef struct utl_storage utl_storage_t;

typedef struct {
    const char *base_path;   ///< Storage root directory path
    bool        encrypt;     ///< Whether to encrypt storage (reserved, not yet implemented)
} utl_storage_cfg_t;

/* ---- Lifecycle ---- */
utl_err_t utl_storage_create(const utl_storage_cfg_t *cfg, utl_storage_t **stg);
void      utl_storage_destroy(utl_storage_t *stg);

/* ---- CRUD ---- */
utl_err_t utl_storage_put(utl_storage_t *stg, const char *key,
                          const void *data, size_t size);
utl_err_t utl_storage_get(utl_storage_t *stg, const char *key,
                          void *buf, size_t buf_size, size_t *read);
utl_err_t utl_storage_remove(utl_storage_t *stg, const char *key);
utl_err_t utl_storage_exists(utl_storage_t *stg, const char *key, bool *exists);

/* ---- Batch operations ---- */
utl_err_t utl_storage_list(utl_storage_t *stg, char keys[][256],
                           size_t max_keys, size_t *count);

UTL_EXTERN_C_END

#endif /* UTL_STORAGE_H */
