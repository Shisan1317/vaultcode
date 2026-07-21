/**
 * @file    search.h
 * @brief   Full-text search — inverted index
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/* Search results */
#define SEARCH_MAX_RESULTS  128
#define SEARCH_MAX_TOKEN    128

typedef struct {
    char  path[512];
    char  title[256];
    char  snippet[256];    /* Match context snippet */
    int   score;           /* Relevance score */
} search_result_t;

typedef struct search search_t;

/* ---- API ---- */

utl_err_t search_create(search_t **s);
void      search_destroy(search_t *s);

/// @brief Index a note
utl_err_t search_index(search_t *s, const char *path, const char *content);

/// @brief Remove a note from index
utl_err_t search_remove(search_t *s, const char *path);

/// @brief Full-text search (AND semantics)
utl_err_t search_query(search_t *s, const char *query,
                       search_result_t *results, int max_results,
                       int *count);

/// @brief Clear all indexes
void search_clear(search_t *s);

UTL_EXTERN_C_END

#endif /* SEARCH_H */
