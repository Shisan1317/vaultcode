/**
 * @file    os_file.h
 * @brief   Cross-platform file I/O abstraction interface — cross-project reusable
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Provides unified file read/write interfaces, abstracting away POSIX / Win32 differences
 *   - Uses opaque handle (os_file_t), external code holds only a pointer
 */

#ifndef OS_FILE_H
#define OS_FILE_H

#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Types & constants
 *===========================================================================*/

/// @brief File handle (opaque pointer)
typedef struct os_file os_file_t;

/// @brief File open mode
typedef enum {
    OS_FILE_MODE_READ       = 0,   ///< Read-only (file must exist)
    OS_FILE_MODE_WRITE      = 1,   ///< Write-only (create/truncate)
    OS_FILE_MODE_APPEND     = 2,   ///< Append (create if not exists)
    OS_FILE_MODE_READ_WRITE = 3,   ///< Read-write (file must exist)
} os_file_mode_t;

/// @brief File seek origin
typedef enum {
    OS_FILE_SEEK_SET  = 0,         ///< From beginning of file
    OS_FILE_SEEK_CUR  = 1,         ///< From current position
    OS_FILE_SEEK_END  = 2,         ///< From end of file
} os_file_seek_t;

/*===========================================================================
 * API
 *===========================================================================*/

/**
 * @brief Open a file
 * @param[out] file  Output file handle
 * @param[in]  path  File path
 * @param[in]  mode  Open mode
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_MEM / UTL_ERR_IO / UTL_ERR_NOT_FOUND
 */
utl_err_t os_file_open(os_file_t **file, const char *path, os_file_mode_t mode);

/**
 * @brief Read data from file
 * @param[in]  file       File handle
 * @param[out] buf        Read buffer
 * @param[in]  size       Expected number of bytes to read
 * @param[out] bytes_read Actual number of bytes read (can be NULL)
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_IO
 */
utl_err_t os_file_read(os_file_t *file, void *buf, size_t size,
                       size_t *bytes_read);

/**
 * @brief Write data to file
 * @param[in] file  File handle
 * @param[in] buf   Data buffer
 * @param[in] size  Number of bytes to write
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_IO
 */
utl_err_t os_file_write(os_file_t *file, const void *buf, size_t size);

/**
 * @brief Get file size
 * @param[in]  file  File handle
 * @param[out] size  Output file size in bytes
 * @return UTL_OK / UTL_ERR_NULL
 */
utl_err_t os_file_size(os_file_t *file, size_t *size);

/**
 * @brief Move file read/write position
 * @param[in] file    File handle
 * @param[in] offset  Offset
 * @param[in] whence  Reference position
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_IO
 */
utl_err_t os_file_seek(os_file_t *file, int64_t offset, os_file_seek_t whence);

/**
 * @brief Force flush buffer to disk
 * @param[in] file  File handle
 * @return UTL_OK / UTL_ERR_NULL / UTL_ERR_IO
 */
utl_err_t os_file_flush(os_file_t *file);

/**
 * @brief Close file
 * @param[in] file  File handle (passing NULL is safely ignored)
 */
void os_file_close(os_file_t *file);

/**
 * @brief Check if a file/directory exists (standalone function, no open required)
 * @param[in] path  Path string
 * @return UTL_TRUE if exists, UTL_FALSE if not exists or argument is null
 */
utl_bool_t os_file_exists(const char *path);

UTL_EXTERN_C_END

#endif /* OS_FILE_H */
