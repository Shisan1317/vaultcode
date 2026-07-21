/**
 * @file    utl_err.h
 * @brief   Global unified error code definitions — cross-module, cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - All modules use this unified error code enum for consistent error logging and tracking
 *   - Each module can map its local enum values to global error codes in its own header
 *   - This header is zero-dependency and can be included by any module
 *
 * Usage example:
 * @code
 *   utl_err_t ret = some_function();
 *   if (UTL_FAIL(ret)) {
 *       printf("error: %d\n", ret);
 *       return ret;
 *   }
 * @endcode
 */

#ifndef UTL_ERR_H
#define UTL_ERR_H

/*===========================================================================
 * Global unified error code enum
 *===========================================================================*/

/// @brief Global operation return codes (negative for errors, zero for success)
typedef enum {
    UTL_OK              =  0,   ///< Operation succeeded

    /* ---- General errors (-1 ~ -9) ---- */
    UTL_ERR_NULL        = -1,   ///< Invalid argument (null pointer/handle)
    UTL_ERR_MEM         = -2,   ///< Memory allocation failed
    UTL_ERR_FULL        = -3,   ///< Capacity full (container/buffer overflow)
    UTL_ERR_EMPTY       = -4,   ///< Container empty (no element to return)
    UTL_ERR_NOT_FOUND   = -5,   ///< Target not found (element/file/resource does not exist)
    UTL_ERR_LOCK        = -6,   ///< Lock operation failed (mutex/spinlock error)
    UTL_ERR_TIMEOUT     = -7,   ///< Operation timed out
    UTL_ERR_IO          = -8,   ///< I/O error (file read/write, network I/O)
    UTL_ERR_NOT_IMPL    = -9,   ///< Feature not implemented (platform not supported yet)

    /* ---- Reserved extension range (-10 ~ -19) ---- */
    UTL_ERR_INVALID     = -10,  ///< Invalid argument value (non-null but out of range / format error)
    UTL_ERR_BUSY        = -11,  ///< Resource busy (already in use)
    UTL_ERR_PERM        = -12,  ///< Permission denied
    UTL_ERR_EXIST       = -13,  ///< Resource already exists
    UTL_ERR_INTERNAL    = -14,  ///< Internal error (unexpected state)

    /* ---- Module-defined error start offset ---- */
    UTL_ERR_MODULE_BASE = -100  ///< Module-specific error codes extend downward from this value
} utl_err_t;

/*===========================================================================
 * Convenience check macros
 *===========================================================================*/

/// @brief Check if operation succeeded (return value is UTL_OK)
#define UTL_SUCC(e)   ((e) == UTL_OK)

/// @brief Check if operation failed (return value is not UTL_OK)
#define UTL_FAIL(e)   ((e) != UTL_OK)

/// @brief Return the string description of an error code
static inline const char *utl_err_str(utl_err_t err)
{
    switch (err) {
    case UTL_OK:            return "OK";
    case UTL_ERR_NULL:      return "NULL pointer";
    case UTL_ERR_MEM:       return "Out of memory";
    case UTL_ERR_FULL:      return "Capacity full";
    case UTL_ERR_EMPTY:     return "Container empty";
    case UTL_ERR_NOT_FOUND: return "Not found";
    case UTL_ERR_LOCK:      return "Lock failed";
    case UTL_ERR_TIMEOUT:   return "Timeout";
    case UTL_ERR_IO:        return "I/O error";
    case UTL_ERR_NOT_IMPL:  return "Not implemented";
    case UTL_ERR_INVALID:   return "Invalid argument";
    case UTL_ERR_BUSY:      return "Resource busy";
    case UTL_ERR_PERM:      return "Permission denied";
    case UTL_ERR_EXIST:     return "Already exists";
    case UTL_ERR_INTERNAL:  return "Internal error";
    default:                return "Unknown error";
    }
}

#endif /* UTL_ERR_H */
