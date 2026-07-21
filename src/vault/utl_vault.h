/**
 * @file    utl_vault.h
 * @brief   Vault 核心业务层 — 统一数据管理 + 加密存储 API
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * 说明:
 *   - 对外提供统一的 put/get/remove API
 *   - 内部组合 storage 和 crypto 层
 *   - 支持数据加密存储(通过 HMAC 摘要校验完整性)
 *   - Vault 是项目的顶层业务模块,app 层仅调用 vault 接口
 */

#ifndef UTL_VAULT_H
#define UTL_VAULT_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

typedef struct utl_vault utl_vault_t;

typedef struct {
    const char *storage_path;  ///< 持久化路径
    bool        encrypt;       ///< 是否启用加密
    const char *secret_key;    ///< 加密密钥(encrypt=true时必填)
} utl_vault_cfg_t;

/* ---- 生命周期 ---- */
utl_err_t utl_vault_create(const utl_vault_cfg_t *cfg, utl_vault_t **vault);
void      utl_vault_destroy(utl_vault_t *vault);

/* ---- 核心 API ---- */

/// @brief 存储数据(自动加密+完整性校验)
utl_err_t utl_vault_put(utl_vault_t *vault, const char *key,
                        const void *data, size_t size);

/// @brief 读取数据(自动解密+完整性校验)
utl_err_t utl_vault_get(utl_vault_t *vault, const char *key,
                        void *buf, size_t buf_size, size_t *read);

/// @brief 删除数据
utl_err_t utl_vault_remove(utl_vault_t *vault, const char *key);

/// @brief 检查数据是否存在
utl_err_t utl_vault_exists(utl_vault_t *vault, const char *key, bool *exists);

UTL_EXTERN_C_END

#endif /* UTL_VAULT_H */
