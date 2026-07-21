/**
 * @file    utl_string.c
 * @brief   String utility implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>

#include "utl_string.h"

/* ---- Basic operations ---- */

utl_err_t utl_str_copy(char *dst, const char *src, size_t dst_size)
{
    if (!dst || !src || dst_size == 0) return UTL_ERR_NULL;
    size_t n = strlen(src);
    if (n >= dst_size) n = dst_size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
    return UTL_OK;
}

utl_err_t utl_str_cat(char *dst, const char *src, size_t dst_size)
{
    if (!dst || !src || dst_size == 0) return UTL_ERR_NULL;
    size_t dlen = strlen(dst);
    if (dlen >= dst_size) return UTL_ERR_FULL;
    size_t remain = dst_size - dlen;
    size_t slen = strlen(src);
    if (slen >= remain) slen = remain - 1;
    memcpy(dst + dlen, src, slen);
    dst[dlen + slen] = '\0';
    return UTL_OK;
}

size_t utl_str_len(const char *str) { return str ? strlen(str) : 0; }
bool utl_str_empty(const char *str) { return (str == NULL || str[0] == '\0'); }

int utl_str_cmp(const char *a, const char *b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

int utl_str_casecmp(const char *a, const char *b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;
#if defined(_MSC_VER)
    return _stricmp(a, b);
#else
    return strcasecmp(a, b);
#endif
}

/* ---- Search ---- */

const char *utl_str_find(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    return strstr(haystack, needle);
}

const char *utl_str_rfind(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen == 0) return haystack + hlen;
    if (nlen > hlen) return NULL;
    for (const char *p = haystack + hlen - nlen; p >= haystack; p--) {
        if (memcmp(p, needle, nlen) == 0) return p;
    }
    return NULL;
}

bool utl_str_startswith(const char *str, const char *prefix)
{
    if (!str || !prefix) return false;
    size_t slen = strlen(str), plen = strlen(prefix);
    if (plen > slen) return false;
    return (memcmp(str, prefix, plen) == 0);
}

bool utl_str_endswith(const char *str, const char *suffix)
{
    if (!str || !suffix) return false;
    size_t slen = strlen(str), sulen = strlen(suffix);
    if (sulen > slen) return false;
    return (memcmp(str + slen - sulen, suffix, sulen) == 0);
}

utl_err_t utl_str_left(const char *str, size_t n, char *buf, size_t buf_size)
{
    if (!str || !buf || buf_size == 0) return UTL_ERR_NULL;
    size_t slen = strlen(str);
    if (n > slen) n = slen;
    if (n >= buf_size) n = buf_size - 1;
    memcpy(buf, str, n);
    buf[n] = '\0';
    return UTL_OK;
}

utl_err_t utl_str_right(const char *str, size_t n, char *buf, size_t buf_size)
{
    if (!str || !buf || buf_size == 0) return UTL_ERR_NULL;
    size_t slen = strlen(str);
    if (n > slen) n = slen;
    if (n >= buf_size) n = buf_size - 1;
    memcpy(buf, str + slen - n, n);
    buf[n] = '\0';
    return UTL_OK;
}

utl_err_t utl_str_substr(const char *str, size_t start, size_t end,
                         char *buf, size_t buf_size)
{
    if (!str || !buf || buf_size == 0) return UTL_ERR_NULL;
    size_t slen = strlen(str);
    if (start > slen || end > slen || start >= end) return UTL_ERR_INVALID;
    size_t n = end - start;
    if (n >= buf_size) n = buf_size - 1;
    memcpy(buf, str + start, n);
    buf[n] = '\0';
    return UTL_OK;
}

/* ---- Transform ---- */

char *utl_str_trim(char *str)
{
    if (!str) return NULL;
    /* Left trim */
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    /* Right trim */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

char *utl_str_tolower(char *str)
{
    if (!str) return NULL;
    for (char *p = str; *p; p++) *p = (char)tolower((unsigned char)*p);
    return str;
}

char *utl_str_toupper(char *str)
{
    if (!str) return NULL;
    for (char *p = str; *p; p++) *p = (char)toupper((unsigned char)*p);
    return str;
}

utl_err_t utl_str_replace(const char *str, const char *old, const char *new_str,
                          char *buf, size_t buf_size)
{
    if (!str || !old || !new_str || !buf || buf_size == 0) return UTL_ERR_NULL;
    size_t old_len = strlen(old), new_len = strlen(new_str);
    size_t written = 0;
    const char *p = str;
    while (*p && written < buf_size - 1) {
        const char *found = strstr(p, old);
        if (!found) {
            size_t remain = strlen(p);
            if (written + remain >= buf_size) remain = buf_size - written - 1;
            memcpy(buf + written, p, remain);
            written += remain;
            break;
        }
        size_t pre_len = (size_t)(found - p);
        if (pre_len > 0) {
            if (written + pre_len >= buf_size) pre_len = buf_size - written - 1;
            memcpy(buf + written, p, pre_len);
            written += pre_len;
        }
        if (written + new_len >= buf_size) new_len = buf_size - written - 1;
        memcpy(buf + written, new_str, new_len);
        written += new_len;
        p = found + old_len;
    }
    buf[written] = '\0';
    return UTL_OK;
}

/* ---- Split & Join ---- */

size_t utl_str_split(const char *str, char delimiter,
                     char tokens[][256], size_t max_tokens)
{
    if (!str || !tokens || max_tokens == 0) return 0;
    size_t count = 0;
    const char *start = str;
    const char *p = str;
    while (*p && count < max_tokens) {
        if (*p == delimiter) {
            size_t n = (size_t)(p - start);
            if (n > 255) n = 255;
            memcpy(tokens[count], start, n);
            tokens[count][n] = '\0';
            count++;
            start = p + 1;
        }
        p++;
    }
    if (count < max_tokens && start < p) {
        size_t n = (size_t)(p - start);
        if (n > 255) n = 255;
        memcpy(tokens[count], start, n);
        tokens[count][n] = '\0';
        count++;
    }
    return count;
}

utl_err_t utl_str_join(const char *parts[], size_t count, char delimiter,
                       char *buf, size_t buf_size)
{
    if (!parts || !buf || buf_size == 0) return UTL_ERR_NULL;
    size_t written = 0;
    for (size_t i = 0; i < count; i++) {
        if (!parts[i]) continue;
        size_t len = strlen(parts[i]);
        if (written + len >= buf_size) len = buf_size - written - 1;
        memcpy(buf + written, parts[i], len);
        written += len;
        if (i < count - 1 && written < buf_size - 1) {
            buf[written++] = delimiter;
        }
    }
    buf[written] = '\0';
    return UTL_OK;
}

/* ---- Format ---- */

utl_err_t utl_str_format(char *buf, size_t buf_size, const char *fmt, ...)
{
    if (!buf || !fmt || buf_size == 0) return UTL_ERR_NULL;
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    if (n < 0) return UTL_ERR_INTERNAL;
    if ((size_t)n >= buf_size) return UTL_ERR_FULL;
    return UTL_OK;
}

utl_err_t utl_str_from_int(int64_t val, char *buf, size_t buf_size)
{
    return utl_str_format(buf, buf_size, "%lld", (long long)val);
}

utl_err_t utl_str_to_int(const char *str, int64_t *val)
{
    if (!str || !val) return UTL_ERR_NULL;
    char *end = NULL;
    long long v = strtoll(str, &end, 0);
    if (end == str) return UTL_ERR_INVALID;
    *val = (int64_t)v;
    return UTL_OK;
}

utl_err_t utl_str_hex_to_bytes(const char *hex, utl_byte_t *out, size_t out_size)
{
    if (!hex || !out) return UTL_ERR_NULL;
    size_t len = strlen(hex);
    if (len % 2 != 0) return UTL_ERR_INVALID;
    if (len / 2 > out_size) return UTL_ERR_FULL;
    for (size_t i = 0; i < len; i += 2) {
        unsigned int byte;
        if (sscanf(hex + i, "%2x", &byte) != 1) return UTL_ERR_INVALID;
        out[i / 2] = (utl_byte_t)byte;
    }
    return UTL_OK;
}

utl_err_t utl_str_bytes_to_hex(const utl_byte_t *in, size_t in_size,
                               char *hex, size_t hex_size)
{
    if (!in || !hex) return UTL_ERR_NULL;
    if (hex_size < in_size * 2 + 1) return UTL_ERR_FULL;
    for (size_t i = 0; i < in_size; i++) {
        snprintf(hex + i * 2, 3, "%02x", in[i]);
    }
    hex[in_size * 2] = '\0';
    return UTL_OK;
}
