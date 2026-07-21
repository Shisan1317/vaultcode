/**
 * @file    os_thread.h
 * @brief   Cross-platform thread abstraction interface — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides unified thread create/join/detach interfaces
 *   - Uses pthread / CreateThread etc. per platform
 */

#ifndef OS_THREAD_H
#define OS_THREAD_H

#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Type definitions
 *===========================================================================*/

/// @brief Thread handle (opaque pointer)
typedef struct os_thread os_thread_t;

/// @brief Thread entry function type
typedef void *(*os_thread_func_t)(void *arg);

/*===========================================================================
 * API
 *===========================================================================*/

/**
 * @brief Create and start a new thread
 * @param[out] thread  Output thread handle
 * @param[in]  func    Thread entry function
 * @param[in]  arg     Argument passed to the entry function
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_MEM
 */
utl_err_t os_thread_create(os_thread_t **thread, os_thread_func_t func, void *arg);

/**
 * @brief Wait for thread to finish (blocking)
 * @param[in]  thread     Thread handle
 * @param[out] retval     Thread return value (can be NULL if not needed)
 * @return UTL_OK / UTL_ERR_NULL
 */
utl_err_t os_thread_join(os_thread_t *thread, void **retval);

/**
 * @brief Detach thread (auto-reclaim resources, cannot join afterwards)
 * @param[in] thread  Thread handle
 * @return UTL_OK / UTL_ERR_NULL
 */
utl_err_t os_thread_detach(os_thread_t *thread);

/**
 * @brief Destroy thread handle (must join or detach first)
 * @param[in] thread  Thread handle (passing NULL is safely ignored)
 */
void os_thread_destroy(os_thread_t *thread);

UTL_EXTERN_C_END

#endif /* OS_THREAD_H */
