/**
 * @file    utl_string.h
 * @brief   String utility module — cross-platform safe string operations
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifndef UTL_STRING_H
#define UTL_STRING_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/*===========================================================================
 * Basic operations
 *===========================================================================*/

/// @brief Safe string copy (null-terminated guaranteed)
utl_err_t utl_str_copy(char *dst, const char *src, size_t dst_size);

/// @brief Safe string concatenation
utl_err_t utl_str_cat(char *dst, const char *src, size_t dst_size);

/// @brief String length (O(1) null check, like strlen but safe)
size_t    utl_str_len(const char *str);

/// @brief Check if string is empty (NULL or empty string)
bool      utl_str_empty(const char *str);

/// @brief String comparison (like strcmp, but NULL-safe)
int       utl_str_cmp(const char *a, const char *b);

/// @brief Case-insensitive comparison
int       utl_str_casecmp(const char *a, const char *b);

/*===========================================================================
 * Search & Extract
 *===========================================================================*/

/// @brief Find first occurrence of substring, returns pointer or NULL if not found
const char *utl_str_find(const char *haystack, const char *needle);

/// @brief Find substring from the right
const char *utl_str_rfind(const char *haystack, const char *needle);

/// @brief Check if starts with prefix
bool utl_str_startswith(const char *str, const char *prefix);

/// @brief Check if ends with suffix
bool utl_str_endswith(const char *str, const char *suffix);

/// @brief Extract left n characters into buffer
utl_err_t utl_str_left(const char *str, size_t n, char *buf, size_t buf_size);

/// @brief Extract right n characters into buffer
utl_err_t utl_str_right(const char *str, size_t n, char *buf, size_t buf_size);

/// @brief Extract substring [start, end)
utl_err_t utl_str_substr(const char *str, size_t start, size_t end,
                         char *buf, size_t buf_size);

/*===========================================================================
 * Transform
 *===========================================================================*/

/// @brief Trim leading/trailing whitespace (in-place, returns modified pointer)
char *utl_str_trim(char *str);

/// @brief Convert to lowercase (in-place)
char *utl_str_tolower(char *str);

/// @brief Convert to uppercase (in-place)
char *utl_str_toupper(char *str);

/// @brief String replace (replace all old with new_str, output to buf)
utl_err_t utl_str_replace(const char *str, const char *old, const char *new_str,
                          char *buf, size_t buf_size);

/*===========================================================================
 * Split & Join
 *===========================================================================*/

/// @brief Maximum number of split segments
#define UTL_STR_SPLIT_MAX  64

/// @brief Split string by delimiter, store results in tokens array
/// @return Actual number of split segments (<= UTL_STR_SPLIT_MAX)
size_t utl_str_split(const char *str, char delimiter,
                     char tokens[][256], size_t max_tokens);

/// @brief Join split segments
utl_err_t utl_str_join(const char *parts[], size_t count, char delimiter,
                       char *buf, size_t buf_size);

/*===========================================================================
 * Format
 *===========================================================================*/

/// @brief Safe formatted write (snprintf wrapper)
utl_err_t utl_str_format(char *buf, size_t buf_size, const char *fmt, ...)
    UTL_PRINTF_LIKE(3, 4);

/// @brief Integer to string
utl_err_t utl_str_from_int(int64_t val, char *buf, size_t buf_size);

/// @brief String to integer
utl_err_t utl_str_to_int(const char *str, int64_t *val);

/// @brief Hex string to byte array
utl_err_t utl_str_hex_to_bytes(const char *hex, utl_byte_t *out, size_t out_size);

/// @brief Byte array to hex string
utl_err_t utl_str_bytes_to_hex(const utl_byte_t *in, size_t in_size,
                               char *hex, size_t hex_size);

UTL_EXTERN_C_END

#endif /* UTL_STRING_H */
