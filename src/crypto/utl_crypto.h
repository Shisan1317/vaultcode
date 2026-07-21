/**
 * @file    utl_crypto.h
 * @brief   Cryptography module — CRC32 / MD5 / SHA256 pure C implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifndef UTL_CRYPTO_H
#define UTL_CRYPTO_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * CRC32 (IEEE 802.3 polynomial)
 *===========================================================================*/

/// @brief Compute CRC32 checksum (incremental, can be called multiple times to append data)
/// @param crc  Initial value (pass 0xFFFFFFFF on first call)
/// @param data Data pointer
/// @param len  Data length
/// @return Updated CRC value (XOR with 0xFFFFFFFF on finalize)
uint32_t utl_crc32_update(uint32_t crc, const void *data, size_t len);

/// @brief One-shot CRC32 computation
static inline uint32_t utl_crc32(const void *data, size_t len)
{
    return utl_crc32_update(0xFFFFFFFF, data, len) ^ 0xFFFFFFFF;
}

/*===========================================================================
 * MD5 — output 128-bit (16 bytes) digest
 *===========================================================================*/

#define UTL_MD5_DIGEST_SIZE  16
#define UTL_MD5_HEX_SIZE     33

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    utl_byte_t buffer[64];
} utl_md5_ctx_t;

void utl_md5_init(utl_md5_ctx_t *ctx);
void utl_md5_update(utl_md5_ctx_t *ctx, const void *data, size_t len);
void utl_md5_final(utl_md5_ctx_t *ctx, utl_byte_t digest[UTL_MD5_DIGEST_SIZE]);

/// @brief One-shot MD5 computation
void utl_md5(const void *data, size_t len, utl_byte_t digest[UTL_MD5_DIGEST_SIZE]);

/// @brief Convert MD5 digest to hex string
void utl_md5_hex(const utl_byte_t digest[UTL_MD5_DIGEST_SIZE],
                 char hex[UTL_MD5_HEX_SIZE]);

/*===========================================================================
 * SHA256 — output 256-bit (32 bytes) digest
 *===========================================================================*/

#define UTL_SHA256_DIGEST_SIZE  32
#define UTL_SHA256_HEX_SIZE     65

typedef struct {
    uint32_t state[8];
    uint64_t count;
    utl_byte_t buffer[64];
} utl_sha256_ctx_t;

void utl_sha256_init(utl_sha256_ctx_t *ctx);
void utl_sha256_update(utl_sha256_ctx_t *ctx, const void *data, size_t len);
void utl_sha256_final(utl_sha256_ctx_t *ctx,
                      utl_byte_t digest[UTL_SHA256_DIGEST_SIZE]);

/// @brief One-shot SHA256 computation
void utl_sha256(const void *data, size_t len,
                utl_byte_t digest[UTL_SHA256_DIGEST_SIZE]);

/// @brief Convert SHA256 digest to hex string
void utl_sha256_hex(const utl_byte_t digest[UTL_SHA256_DIGEST_SIZE],
                    char hex[UTL_SHA256_HEX_SIZE]);

/*===========================================================================
 * HMAC-SHA256
 *===========================================================================*/

void utl_hmac_sha256(const utl_byte_t *key, size_t key_len,
                     const void *data, size_t data_len,
                     utl_byte_t digest[UTL_SHA256_DIGEST_SIZE]);

UTL_EXTERN_C_END

#endif /* UTL_CRYPTO_H */
