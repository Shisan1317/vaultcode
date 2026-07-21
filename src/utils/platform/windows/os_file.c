/**
 * @file    os_file.c (Windows stub)
 * @brief   文件 I/O — Windows CreateFile 桩实现
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>

#include "os_file.h"

struct os_file {
    int placeholder;
};

utl_err_t os_file_open(os_file_t **file, const char *path, os_file_mode_t mode)
{
    UTL_UNUSED(file);
    UTL_UNUSED(path);
    UTL_UNUSED(mode);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_file_read(os_file_t *file, void *buf, size_t size,
                       size_t *bytes_read)
{
    UTL_UNUSED(file);
    UTL_UNUSED(buf);
    UTL_UNUSED(size);
    UTL_UNUSED(bytes_read);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_file_write(os_file_t *file, const void *buf, size_t size)
{
    UTL_UNUSED(file);
    UTL_UNUSED(buf);
    UTL_UNUSED(size);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_file_size(os_file_t *file, size_t *size)
{
    UTL_UNUSED(file);
    UTL_UNUSED(size);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_file_seek(os_file_t *file, int64_t offset, os_file_seek_t whence)
{
    UTL_UNUSED(file);
    UTL_UNUSED(offset);
    UTL_UNUSED(whence);
    return UTL_ERR_NOT_IMPL;
}

utl_err_t os_file_flush(os_file_t *file)
{
    UTL_UNUSED(file);
    return UTL_ERR_NOT_IMPL;
}

void os_file_close(os_file_t *file)
{
    UTL_UNUSED(file);
}

utl_bool_t os_file_exists(const char *path)
{
    UTL_UNUSED(path);
    return UTL_FALSE;
}
