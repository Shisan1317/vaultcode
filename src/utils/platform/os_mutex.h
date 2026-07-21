/**
 * @file    os_mutex.h
 * @brief   Cross-platform mutex abstraction interface — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides a unified mutex interface, using pthread / CRITICAL_SECTION etc. per platform
 *   - Uses opaque handle (os_mutex_t), external code holds only a pointer, internal structure fully hidden
 *   - In non-thread-safe scenarios, NULL mutex handle can be passed directly. Lock operations degenerate to no-ops.
 */

#ifndef OS_MUTEX_H
#define OS_MUTEX_H

#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Type forward declarations
 *===========================================================================*/

/// @brief Mutex handle (opaque pointer)
typedef struct os_mutex os_mutex_t;

/*===========================================================================
 * API
 *===========================================================================*/

/**
 * @brief Create mutex instance
 * @param[out] mutex  Output mutex handle (heap-allocated)
 * @return UTL_OK on success, UTL_ERR_NULL invalid argument, UTL_ERR_MEM out of memory
 */
utl_err_t os_mutex_create(os_mutex_t **mutex);

/**
 * @brief Lock (block until lock is acquired)
 * @param[in] mutex  Lock handle (can be NULL, degenerates to no-op returning OK)
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_LOCK
 */
utl_err_t os_mutex_lock(os_mutex_t *mutex);

/**
 * @brief Unlock
 * @param[in] mutex  Lock handle (can be NULL, degenerates to no-op returning OK)
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_LOCK
 */
utl_err_t os_mutex_unlock(os_mutex_t *mutex);

/**
 * @brief Destroy mutex instance
 * @param[in] mutex  Lock handle (passing NULL is safely ignored)
 */
void os_mutex_destroy(os_mutex_t *mutex);

UTL_EXTERN_C_END

#endif /* OS_MUTEX_H */
