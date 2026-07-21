/**
 * @file    utl_vault.c
 * @brief   Vault 核心业务层实现 — 加密持久化存储
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * 实现概要:
 *   1. utl_vault_put: data → HMAC-SHA256(data) → [hmac(32B) | data] → storage
 *   2. utl_vault_get: storage → [hmac(32B) | data] → 校验HMAC → 返回data
 *   3. 如果encrypt开启,会用AES(预留)加密后再存入
 */

#include <stdlib.h>
#include <string.h>

#include "utl_vault.h"
#include "utl_storage.h"
#include "utl_crypto.h"
#include "utl_string.h"

#define VAULT_HMAC_SIZE    UTL_SHA256_DIGEST_SIZE
#define VAULT_MAX_DATA     (10 * 1024 * 1024)  /* 10MB上限 */

struct utl_vault {
    utl_storage_t *storage;
    bool           encrypt;
    utl_byte_t     secret_key[32];  /* SHA256(用户密钥) */
    char           secret_hex[UTL_SHA256_HEX_SIZE];
};

static const utl_vault_cfg_t kDefaultCfg = {
    .storage_path = "target/storage",
    .encrypt      = false,
    .secret_key   = NULL,
};

static void prv_derive_key(utl_vault_t *v, const char *secret)
{
    if (secret && secret[0]) {
        utl_sha256(secret, strlen(secret), v->secret_key);
        utl_sha256_hex(v->secret_key, v->secret_hex);
    } else {
        memset(v->secret_key, 0, 32);
        v->secret_hex[0] = '\0';
    }
}

/* 封包: [HMAC(32B) | payload] */
static utl_err_t prv_pack(utl_vault_t *v, const void *data, size_t size,
                          utl_byte_t **packed, size_t *packed_size)
{
    *packed_size = VAULT_HMAC_SIZE + size;
    *packed = (utl_byte_t *)malloc(*packed_size);
    if (!*packed) return UTL_ERR_MEM;

    /* 计算 HMAC */
    utl_byte_t hmac[VAULT_HMAC_SIZE];
    const utl_byte_t *key = (v->encrypt && v->secret_hex[0]) ? v->secret_key : NULL;
    size_t key_len = (v->encrypt && v->secret_hex[0]) ? 32 : 0;

    if (key_len > 0) {
        utl_hmac_sha256(key, key_len, data, size, hmac);
    } else {
        /* 无密钥时使用固定salt做完整性校验 */
        utl_hmac_sha256((const utl_byte_t *)"vaultcode", 9, data, size, hmac);
    }

    memcpy(*packed, hmac, VAULT_HMAC_SIZE);
    memcpy(*packed + VAULT_HMAC_SIZE, data, size);
    return UTL_OK;
}

/* 解包: 校验HMAC,提取payload */
static utl_err_t prv_unpack(utl_vault_t *v, const utl_byte_t *packed,
                            size_t packed_size, void *buf, size_t buf_size,
                            size_t *read)
{
    if (packed_size <= VAULT_HMAC_SIZE) return UTL_ERR_IO;

    size_t payload_size = packed_size - VAULT_HMAC_SIZE;
    const utl_byte_t *hmac    = packed;
    const utl_byte_t *payload = packed + VAULT_HMAC_SIZE;

    /* 校验 HMAC */
    utl_byte_t calc_hmac[VAULT_HMAC_SIZE];
    const utl_byte_t *key = (v->encrypt && v->secret_hex[0]) ? v->secret_key : NULL;
    size_t key_len = (v->encrypt && v->secret_hex[0]) ? 32 : 0;

    if (key_len > 0) {
        utl_hmac_sha256(key, key_len, payload, payload_size, calc_hmac);
    } else {
        utl_hmac_sha256((const utl_byte_t *)"vaultcode", 9, payload, payload_size, calc_hmac);
    }

    if (memcmp(hmac, calc_hmac, VAULT_HMAC_SIZE) != 0) return UTL_ERR_IO;

    size_t n = payload_size < buf_size ? payload_size : buf_size;
    memcpy(buf, payload, n);
    if (read) *read = n;
    return UTL_OK;
}

/* ---- API ---- */

utl_err_t utl_vault_create(const utl_vault_cfg_t *cfg, utl_vault_t **vault)
{
    if (!vault) return UTL_ERR_NULL;
    const utl_vault_cfg_t *c = cfg ? cfg : &kDefaultCfg;

    utl_vault_t *v = (utl_vault_t *)malloc(sizeof(utl_vault_t));
    if (!v) return UTL_ERR_MEM;

    v->encrypt = c->encrypt;
    prv_derive_key(v, c->secret_key);

    utl_storage_cfg_t scfg = {
        .base_path = c->storage_path,
        .encrypt   = false,
    };

    utl_err_t ret = utl_storage_create(&scfg, &v->storage);
    if (UTL_FAIL(ret)) { free(v); return ret; }

    *vault = v;
    return UTL_OK;
}

void utl_vault_destroy(utl_vault_t *vault)
{
    if (!vault) return;
    if (vault->storage) utl_storage_destroy(vault->storage);
    memset(vault->secret_key, 0, 32);
    free(vault);
}

utl_err_t utl_vault_put(utl_vault_t *vault, const char *key,
                        const void *data, size_t size)
{
    if (!vault || !key || !data || size == 0) return UTL_ERR_NULL;
    if (size > VAULT_MAX_DATA) return UTL_ERR_FULL;

    utl_byte_t *packed  = NULL;
    size_t      psize   = 0;
    utl_err_t   ret     = prv_pack(vault, data, size, &packed, &psize);
    if (UTL_FAIL(ret)) return ret;

    ret = utl_storage_put(vault->storage, key, packed, psize);
    free(packed);
    return ret;
}

utl_err_t utl_vault_get(utl_vault_t *vault, const char *key,
                        void *buf, size_t buf_size, size_t *read)
{
    if (!vault || !key || !buf || buf_size == 0) return UTL_ERR_NULL;

    /* 多分配 HMAC_SIZE 空间读入完整包 */
    size_t max_psize = VAULT_MAX_DATA + VAULT_HMAC_SIZE;
    utl_byte_t *packed = (utl_byte_t *)malloc(max_psize);
    if (!packed) return UTL_ERR_MEM;

    size_t psize = 0;
    utl_err_t ret = utl_storage_get(vault->storage, key, packed, max_psize, &psize);
    if (UTL_FAIL(ret)) { free(packed); return ret; }

    ret = prv_unpack(vault, packed, psize, buf, buf_size, read);
    free(packed);
    return ret;
}

utl_err_t utl_vault_remove(utl_vault_t *vault, const char *key)
{
    if (!vault || !key) return UTL_ERR_NULL;
    return utl_storage_remove(vault->storage, key);
}

utl_err_t utl_vault_exists(utl_vault_t *vault, const char *key, bool *exists)
{
    if (!vault || !key || !exists) return UTL_ERR_NULL;
    return utl_storage_exists(vault->storage, key, exists);
}
