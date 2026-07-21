/**
 * @file    utl_storage.c
 * @brief   Storage layer implementation — file-based key-value storage
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "utl_storage.h"
#include "os_file.h"
#include "os_mutex.h"
#include "utl_string.h"
#include "utl_crypto.h"

struct utl_storage {
    char        base_path[512];
    bool        encrypt;
    os_mutex_t *mutex;
};

/* Default path: target/storage/ */
#define STORAGE_DEFAULT_PATH  "target/storage"

static const utl_storage_cfg_t kDefaultCfg = {
    .base_path = STORAGE_DEFAULT_PATH,
    .encrypt   = false,
};

/* Build the full file path */
static void prv_key_path(utl_storage_t *stg, const char *key,
                         char *path, size_t path_size)
{
    /* Hash key with SHA256 to generate a unique filename, avoiding special character issues */
    utl_byte_t hash[UTL_SHA256_DIGEST_SIZE];
    char hash_hex[UTL_SHA256_HEX_SIZE];
    utl_sha256(key, strlen(key), hash);
    utl_sha256_hex(hash, hash_hex);

    snprintf(path, path_size, "%s/%s.dat", stg->base_path, hash_hex);
}

utl_err_t utl_storage_create(const utl_storage_cfg_t *cfg, utl_storage_t **stg)
{
    if (!stg) return UTL_ERR_NULL;
    const utl_storage_cfg_t *c = cfg ? cfg : &kDefaultCfg;

    utl_storage_t *s = (utl_storage_t *)malloc(sizeof(utl_storage_t));
    if (!s) return UTL_ERR_MEM;

    utl_str_copy(s->base_path, c->base_path, sizeof(s->base_path));
    s->encrypt = c->encrypt;
    s->mutex   = NULL;

    /* Ensure directory exists */
    mkdir(s->base_path, 0755);

    utl_err_t ret = os_mutex_create(&s->mutex);
    if (UTL_FAIL(ret)) { free(s); return ret; }

    *stg = s;
    return UTL_OK;
}

void utl_storage_destroy(utl_storage_t *stg)
{
    if (!stg) return;
    if (stg->mutex) os_mutex_destroy(stg->mutex);
    free(stg);
}

utl_err_t utl_storage_put(utl_storage_t *stg, const char *key,
                          const void *data, size_t size)
{
    if (!stg || !key || !data || size == 0) return UTL_ERR_NULL;

    char path[1024];
    prv_key_path(stg, key, path, sizeof(path));

    os_mutex_lock(stg->mutex);

    os_file_t *file = NULL;
    utl_err_t ret = os_file_open(&file, path, OS_FILE_MODE_WRITE);
    if (UTL_FAIL(ret)) { os_mutex_unlock(stg->mutex); return ret; }

    /* File header: 4-byte CRC32 + data */
    uint32_t crc = utl_crc32(data, size);
    os_file_write(file, &crc, sizeof(crc));
    ret = os_file_write(file, data, size);

    os_file_close(file);
    os_mutex_unlock(stg->mutex);
    return ret;
}

utl_err_t utl_storage_get(utl_storage_t *stg, const char *key,
                          void *buf, size_t buf_size, size_t *read)
{
    if (!stg || !key || !buf || !buf_size) return UTL_ERR_NULL;

    char path[1024];
    prv_key_path(stg, key, path, sizeof(path));

    os_mutex_lock(stg->mutex);

    os_file_t *file = NULL;
    utl_err_t ret = os_file_open(&file, path, OS_FILE_MODE_READ);
    if (UTL_FAIL(ret)) { os_mutex_unlock(stg->mutex); return ret; }

    /* Get file size */
    size_t fsize = 0;
    os_file_size(file, &fsize);
    if (fsize <= sizeof(uint32_t)) { os_file_close(file); os_mutex_unlock(stg->mutex); return UTL_ERR_IO; }

    /* Read CRC */
    uint32_t stored_crc = 0;
    os_file_read(file, &stored_crc, sizeof(stored_crc), NULL);

    /* Read data */
    size_t data_size = fsize - sizeof(uint32_t);
    if (data_size > buf_size) data_size = buf_size;
    size_t actual = 0;
    os_file_read(file, buf, data_size, &actual);

    os_file_close(file);
    os_mutex_unlock(stg->mutex);

    /* Verify CRC */
    uint32_t calc_crc = utl_crc32(buf, actual);
    if (stored_crc != calc_crc) return UTL_ERR_IO;

    if (read) *read = actual;
    return UTL_OK;
}

utl_err_t utl_storage_remove(utl_storage_t *stg, const char *key)
{
    if (!stg || !key) return UTL_ERR_NULL;

    char path[1024];
    prv_key_path(stg, key, path, sizeof(path));

    os_mutex_lock(stg->mutex);

    if (unlink(path) != 0) {
        os_mutex_unlock(stg->mutex);
        return UTL_ERR_NOT_FOUND;
    }

    os_mutex_unlock(stg->mutex);
    return UTL_OK;
}

utl_err_t utl_storage_exists(utl_storage_t *stg, const char *key, bool *exists)
{
    if (!stg || !key || !exists) return UTL_ERR_NULL;
    char path[1024];
    prv_key_path(stg, key, path, sizeof(path));
    *exists = os_file_exists(path);
    return UTL_OK;
}

utl_err_t utl_storage_list(utl_storage_t *stg, char keys[][256],
                           size_t max_keys, size_t *count)
{
    if (!stg || !keys || !max_keys || !count) return UTL_ERR_NULL;

    os_mutex_lock(stg->mutex);

    DIR *dir = opendir(stg->base_path);
    if (!dir) { os_mutex_unlock(stg->mutex); return UTL_ERR_IO; }

    size_t n = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && n < max_keys) {
        if (entry->d_type == DT_REG) {
            /* Strip .dat suffix */
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcmp(entry->d_name + len - 4, ".dat") == 0) {
                size_t copy_len = len - 4;
                if (copy_len >= 256) copy_len = 255;
                memcpy(keys[n], entry->d_name, copy_len);
                keys[n][copy_len] = '\0';
                n++;
            }
        }
    }
    closedir(dir);

    os_mutex_unlock(stg->mutex);
    *count = n;
    return UTL_OK;
}
