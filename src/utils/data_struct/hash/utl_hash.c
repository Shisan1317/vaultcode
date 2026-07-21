/**
 * @file    utl_hash.c
 * @brief   General-purpose hash table — chaining method implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include "utl_hash.h"
#include "os_mutex.h"

/* ---- Private structures ---- */
typedef struct hash_node {
    void            *key;
    void            *value;
    struct hash_node *next;
} hash_node_t;

struct utl_hash {
    hash_node_t     **buckets;
    uint32_t          bucket_count;
    utl_hash_key_type_t key_type;
    size_t            size;
    uint32_t          max_size;
    bool              thread_safe;
    os_mutex_t       *mutex;
};

/* ---- Default configuration ---- */
#define HASH_DEFAULT_BUCKETS  509

static const utl_hash_cfg_t kDefaultCfg = {
    .bucket_count = HASH_DEFAULT_BUCKETS,
    .key_type     = UTL_HASH_KEY_INT,
    .thread_safe  = false,
    .max_size     = 0,
};

/* ---- Hash functions ---- */
static inline uint32_t prv_hash_int(uint64_t key)
{
    /* Knuth multiplicative hash */
    return (uint32_t)(key * 2654435761ULL);
}

static uint32_t prv_hash_str(const char *key)
{
    /* DJB2 hash */
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static uint32_t prv_hash(void *key, utl_hash_key_type_t type)
{
    if (type == UTL_HASH_KEY_STR) {
        return prv_hash_str((const char *)key);
    } else {
        /* INT mode: key pointer value itself is used as the integer key */
        return prv_hash_int((uint64_t)(uintptr_t)key);
    }
}

/* ---- Key comparison ---- */
static bool prv_key_eq(void *a, void *b, utl_hash_key_type_t type)
{
    if (type == UTL_HASH_KEY_STR) {
        return (a != NULL && b != NULL && strcmp((const char *)a, (const char *)b) == 0);
    } else {
        return (a == b);
    }
}

/* ---- Lock helpers ---- */
static inline int prv_lock(utl_hash_t *h)
{
    if (h->thread_safe && h->mutex) return os_mutex_lock(h->mutex);
    return 0;
}
static inline int prv_unlock(utl_hash_t *h)
{
    if (h->thread_safe && h->mutex) return os_mutex_unlock(h->mutex);
    return 0;
}

/* ---- API ---- */

utl_err_t utl_hash_create(const utl_hash_cfg_t *cfg, utl_hash_t **hash)
{
    if (hash == NULL) return UTL_ERR_NULL;
    const utl_hash_cfg_t *c = cfg ? cfg : &kDefaultCfg;
    uint32_t n = c->bucket_count > 0 ? c->bucket_count : HASH_DEFAULT_BUCKETS;

    utl_hash_t *h = (utl_hash_t *)malloc(sizeof(utl_hash_t));
    if (!h) return UTL_ERR_MEM;

    h->buckets = (hash_node_t **)calloc(n, sizeof(hash_node_t *));
    if (!h->buckets) { free(h); return UTL_ERR_MEM; }

    h->bucket_count = n;
    h->key_type     = c->key_type;
    h->size         = 0;
    h->max_size     = c->max_size;
    h->thread_safe  = c->thread_safe;
    h->mutex        = NULL;

    if (h->thread_safe) {
        utl_err_t ret = os_mutex_create(&h->mutex);
        if (UTL_FAIL(ret)) { free(h->buckets); free(h); return ret; }
    }

    *hash = h;
    return UTL_OK;
}

void utl_hash_destroy(utl_hash_t *hash)
{
    if (!hash) return;
    utl_hash_clear(hash);
    free(hash->buckets);
    if (hash->thread_safe && hash->mutex) os_mutex_destroy(hash->mutex);
    memset(hash, 0, sizeof(*hash));
    free(hash);
}

utl_err_t utl_hash_put(utl_hash_t *hash, void *key, void *value)
{
    if (!hash || !key) return UTL_ERR_NULL;
    prv_lock(hash);

    if (hash->max_size > 0 && hash->size >= hash->max_size) {
        prv_unlock(hash);
        return UTL_ERR_FULL;
    }

    uint32_t idx = prv_hash(key, hash->key_type) % hash->bucket_count;

    /* Check if already exists (overwrite) */
    hash_node_t *cur = hash->buckets[idx];
    while (cur) {
        if (prv_key_eq(cur->key, key, hash->key_type)) {
            cur->value = value;
            prv_unlock(hash);
            return UTL_OK;
        }
        cur = cur->next;
    }

    /* Create new node */
    hash_node_t *node = (hash_node_t *)malloc(sizeof(hash_node_t));
    if (!node) { prv_unlock(hash); return UTL_ERR_MEM; }
    node->key   = key;
    node->value = value;
    node->next  = hash->buckets[idx];
    hash->buckets[idx] = node;
    hash->size++;

    prv_unlock(hash);
    return UTL_OK;
}

utl_err_t utl_hash_get(const utl_hash_t *hash, void *key, void **value)
{
    if (!hash || !key || !value) return UTL_ERR_NULL;
    prv_lock((utl_hash_t *)hash);

    uint32_t idx = prv_hash(key, hash->key_type) % hash->bucket_count;
    hash_node_t *cur = hash->buckets[idx];
    while (cur) {
        if (prv_key_eq(cur->key, key, hash->key_type)) {
            *value = cur->value;
            prv_unlock((utl_hash_t *)hash);
            return UTL_OK;
        }
        cur = cur->next;
    }

    prv_unlock((utl_hash_t *)hash);
    return UTL_ERR_NOT_FOUND;
}

utl_err_t utl_hash_remove(utl_hash_t *hash, void *key)
{
    if (!hash || !key) return UTL_ERR_NULL;
    prv_lock(hash);

    uint32_t idx = prv_hash(key, hash->key_type) % hash->bucket_count;
    hash_node_t *cur  = hash->buckets[idx];
    hash_node_t *prev = NULL;

    while (cur) {
        if (prv_key_eq(cur->key, key, hash->key_type)) {
            if (prev) prev->next = cur->next;
            else      hash->buckets[idx] = cur->next;
            free(cur);
            hash->size--;
            prv_unlock(hash);
            return UTL_OK;
        }
        prev = cur;
        cur  = cur->next;
    }

    prv_unlock(hash);
    return UTL_ERR_NOT_FOUND;
}

utl_err_t utl_hash_contains(const utl_hash_t *hash, void *key, bool *found)
{
    if (!hash || !key || !found) return UTL_ERR_NULL;
    void *val = NULL;
    utl_err_t ret = utl_hash_get(hash, key, &val);
    *found = UTL_SUCC(ret);
    return UTL_OK;
}

utl_err_t utl_hash_clear(utl_hash_t *hash)
{
    if (!hash) return UTL_ERR_NULL;
    prv_lock(hash);

    for (uint32_t i = 0; i < hash->bucket_count; i++) {
        hash_node_t *cur = hash->buckets[i];
        while (cur) {
            hash_node_t *next = cur->next;
            free(cur);
            cur = next;
        }
        hash->buckets[i] = NULL;
    }
    hash->size = 0;

    prv_unlock(hash);
    return UTL_OK;
}

utl_err_t utl_hash_size(const utl_hash_t *hash, size_t *size)
{
    if (!hash || !size) return UTL_ERR_NULL;
    prv_lock((utl_hash_t *)hash);
    *size = hash->size;
    prv_unlock((utl_hash_t *)hash);
    return UTL_OK;
}

/* ---- Traversal ---- */

utl_err_t utl_hash_iter_begin(const utl_hash_t *hash, utl_hash_iter_t *iter)
{
    if (!hash || !iter) return UTL_ERR_NULL;
    memset(iter, 0, sizeof(*iter));

    for (uint32_t i = 0; i < hash->bucket_count; i++) {
        if (hash->buckets[i]) {
            iter->_bucket = i;
            iter->_node   = hash->buckets[i];
            iter->key     = ((hash_node_t *)iter->_node)->key;
            iter->value   = ((hash_node_t *)iter->_node)->value;
            return UTL_OK;
        }
    }
    iter->_node = NULL;
    return UTL_ERR_EMPTY;
}

utl_err_t utl_hash_iter_next(const utl_hash_t *hash, utl_hash_iter_t *iter)
{
    if (!hash || !iter || !iter->_node) return UTL_ERR_NULL;

    hash_node_t *node = (hash_node_t *)iter->_node;

    /* Next in the same bucket */
    if (node->next) {
        iter->_node = node->next;
        node = node->next;
        iter->key   = node->key;
        iter->value = node->value;
        return UTL_OK;
    }

    /* Next non-empty bucket */
    for (uint32_t i = (uint32_t)(iter->_bucket + 1); i < hash->bucket_count; i++) {
        if (hash->buckets[i]) {
            iter->_bucket = i;
            iter->_node   = hash->buckets[i];
            node = hash->buckets[i];
            iter->key   = node->key;
            iter->value = node->value;
            return UTL_OK;
        }
    }

    iter->_node = NULL;
    return UTL_ERR_NOT_FOUND;
}

bool utl_hash_iter_end(const utl_hash_iter_t *iter)
{
    return (iter == NULL || iter->_node == NULL);
}
