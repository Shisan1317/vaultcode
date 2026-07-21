/**
 * @file    os_file.c (Linux)
 * @brief   File I/O — POSIX fopen/fread/fwrite implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "os_file.h"

struct os_file {
    FILE *fp;
};

/* ---- Mode string conversion ---- */
static const char *prv_mode_str(os_file_mode_t mode)
{
    switch (mode) {
    case OS_FILE_MODE_READ:        return "rb";
    case OS_FILE_MODE_WRITE:       return "wb";
    case OS_FILE_MODE_APPEND:      return "ab";
    case OS_FILE_MODE_READ_WRITE:  return "r+b";
    default:                       return "rb";
    }
}

/* ---- Seek base conversion ---- */
static int prv_whence(os_file_seek_t whence)
{
    switch (whence) {
    case OS_FILE_SEEK_SET:  return SEEK_SET;
    case OS_FILE_SEEK_CUR:  return SEEK_CUR;
    case OS_FILE_SEEK_END:  return SEEK_END;
    default:                return SEEK_SET;
    }
}

/* ---- API ---- */

utl_err_t os_file_open(os_file_t **file, const char *path, os_file_mode_t mode)
{
    if (file == NULL || path == NULL) {
        return UTL_ERR_NULL;
    }

    os_file_t *f = (os_file_t *)malloc(sizeof(os_file_t));
    if (f == NULL) {
        return UTL_ERR_MEM;
    }

    f->fp = fopen(path, prv_mode_str(mode));
    if (f->fp == NULL) {
        free(f);
        /* Determine whether file does not exist or other I/O error */
        struct stat st;
        if (stat(path, &st) != 0) {
            return UTL_ERR_NOT_FOUND;
        }
        return UTL_ERR_IO;
    }

    *file = f;
    return UTL_OK;
}

utl_err_t os_file_read(os_file_t *file, void *buf, size_t size,
                       size_t *bytes_read)
{
    if (file == NULL || buf == NULL || size == 0) {
        return UTL_ERR_NULL;
    }

    size_t n = fread(buf, 1, size, file->fp);
    if (bytes_read != NULL) {
        *bytes_read = n;
    }

    if (n == 0 && ferror(file->fp)) {
        return UTL_ERR_IO;
    }

    return UTL_OK;
}

utl_err_t os_file_write(os_file_t *file, const void *buf, size_t size)
{
    if (file == NULL || buf == NULL || size == 0) {
        return UTL_ERR_NULL;
    }

    size_t n = fwrite(buf, 1, size, file->fp);
    if (n < size) {
        return UTL_ERR_IO;
    }

    return UTL_OK;
}

utl_err_t os_file_size(os_file_t *file, size_t *size)
{
    if (file == NULL || size == NULL) {
        return UTL_ERR_NULL;
    }

    long cur = ftell(file->fp);
    if (cur < 0) {
        return UTL_ERR_IO;
    }

    if (fseek(file->fp, 0, SEEK_END) != 0) {
        return UTL_ERR_IO;
    }

    long end_pos = ftell(file->fp);
    if (end_pos < 0) {
        return UTL_ERR_IO;
    }

    *size = (size_t)end_pos;

    /* Restore original position */
    fseek(file->fp, cur, SEEK_SET);

    return UTL_OK;
}

utl_err_t os_file_seek(os_file_t *file, int64_t offset, os_file_seek_t whence)
{
    if (file == NULL) {
        return UTL_ERR_NULL;
    }

    if (fseek(file->fp, (long)offset, prv_whence(whence)) != 0) {
        return UTL_ERR_IO;
    }

    return UTL_OK;
}

utl_err_t os_file_flush(os_file_t *file)
{
    if (file == NULL) {
        return UTL_ERR_NULL;
    }

    if (fflush(file->fp) != 0) {
        return UTL_ERR_IO;
    }

    return UTL_OK;
}

void os_file_close(os_file_t *file)
{
    if (file == NULL) {
        return;
    }

    fclose(file->fp);
    free(file);
}

utl_bool_t os_file_exists(const char *path)
{
    if (path == NULL) {
        return UTL_FALSE;
    }

    struct stat st;
    return (stat(path, &st) == 0) ? UTL_TRUE : UTL_FALSE;
}
