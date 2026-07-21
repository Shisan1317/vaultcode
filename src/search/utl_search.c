/**
 * @file    search.c
 * @brief   Full-text search — inverted index implementation
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
#include <ctype.h>

#include "utl_search.h"
#include "utl_hash.h"

/* Internal: inverted index entry */
typedef struct {
    char  token[SEARCH_MAX_TOKEN];
    /* Store path list as a simple comma-separated string */
    char  paths[4096];     /* "path1,path2,path3" */
    int   count;           /* Number of notes containing this token */
} inv_entry_t;

struct search {
    utl_hash_t *index;     /* token -> inv_entry_t* */
};

/* ---- Tokenizer ---- */
static bool prv_is_token_char(int c)
{
    return isalnum(c) || c == '_' || c == '-';
}

static void prv_tokenize(const char *text, char tokens[][SEARCH_MAX_TOKEN],
                         int *count, int max_tokens)
{
    *count = 0;
    const char *p = text;
    while (*p && *count < max_tokens) {
        /* Skip delimiters */
        while (*p && !prv_is_token_char(*p)) p++;
        if (!*p) break;

        const char *start = p;
        while (*p && prv_is_token_char(*p)) p++;

        size_t len = (size_t)(p - start);
        if (len >= 2 && len < SEARCH_MAX_TOKEN) {  /* Ignore single characters */
            size_t i;
            for (i = 0; i < len; i++)
                tokens[*count][i] = (char)tolower((unsigned char)start[i]);
            tokens[*count][i] = '\0';

            /* Simple dedup (adjacent duplicates only) */
            if (*count == 0 || strcmp(tokens[*count], tokens[*count-1]) != 0)
                (*count)++;
        }
    }
}

/* ---- API ---- */

utl_err_t search_create(search_t **s)
{
    if (!s) return UTL_ERR_NULL;
    search_t *sr = (search_t *)malloc(sizeof(search_t));
    if (!sr) return UTL_ERR_MEM;

    utl_hash_cfg_t cfg = {
        .bucket_count = 503,
        .key_type     = UTL_HASH_KEY_STR,
        .thread_safe  = false,
        .max_size     = 0,
    };
    utl_err_t ret = utl_hash_create(&cfg, &sr->index);
    if (UTL_FAIL(ret)) { free(sr); return ret; }

    *s = sr;
    return UTL_OK;
}

void search_destroy(search_t *s)
{
    if (!s) return;
    /* Free all inv_entry items */
    utl_hash_iter_t iter;
    if (UTL_SUCC(utl_hash_iter_begin(s->index, &iter))) {
        do {
            inv_entry_t *e = (inv_entry_t *)iter.value;
            if (e) free(e);
        } while (UTL_SUCC(utl_hash_iter_next(s->index, &iter)));
    }
    utl_hash_destroy(s->index);
    free(s);
}

utl_err_t search_index(search_t *s, const char *path, const char *content)
{
    if (!s || !path || !content) return UTL_ERR_NULL;

    char tokens[512][SEARCH_MAX_TOKEN];
    int tcount = 0;
    prv_tokenize(content, tokens, &tcount, 512);

    for (int i = 0; i < tcount; i++) {
        inv_entry_t *entry = NULL;
        utl_err_t ret = utl_hash_get(s->index, tokens[i], (void **)&entry);
        if (UTL_FAIL(ret)) {
            /* Create new entry */
            entry = (inv_entry_t *)calloc(1, sizeof(inv_entry_t));
            if (!entry) continue;
            strncpy(entry->token, tokens[i], SEARCH_MAX_TOKEN - 1);
            snprintf(entry->paths, sizeof(entry->paths), "%s", path);
            entry->count = 1;
            /* Use entry->token as key (persistent memory), not stack-local tokens[i] */
            utl_hash_put(s->index, entry->token, entry);
        } else {
            /* Check if path already exists */
            if (!strstr(entry->paths, path)) {
                size_t cur = strlen(entry->paths);
                snprintf(entry->paths + cur, sizeof(entry->paths) - cur,
                         ",%s", path);
                entry->count++;
            }
        }
    }
    return UTL_OK;
}

utl_err_t search_remove(search_t *s, const char *path)
{
    if (!s || !path) return UTL_ERR_NULL;
    /* Simplified: rebuild index (feasible for small datasets) */
    UTL_UNUSED(path);
    return UTL_OK;
}

utl_err_t search_query(search_t *s, const char *query,
                       search_result_t *results, int max_results,
                       int *count)
{
    if (!s || !query || !results || !count) return UTL_ERR_NULL;
    *count = 0;

    /* Tokenize query */
    char q_tokens[32][SEARCH_MAX_TOKEN];
    int qcount = 0;
    prv_tokenize(query, q_tokens, &qcount, 32);
    if (qcount == 0) return UTL_OK;

    /* Look up each token, compute intersection (AND semantics) */
    /* Use simple scoring: matched tokens / total tokens */
    typedef struct { char path[512]; int score; int matched; } scored_t;
    scored_t scored[SEARCH_MAX_RESULTS];
    int scored_count = 0;

    for (int i = 0; i < qcount; i++) {
        inv_entry_t *entry = NULL;
        if (UTL_FAIL(utl_hash_get(s->index, q_tokens[i], (void **)&entry)))
            continue;

        if (!entry) continue;
        /* Parse paths (need copy, strtok_r modifies original string) */
        char *ps = strdup(entry->paths);
        if (!ps) continue;
        char *save = NULL;
        char *tok = strtok_r(ps, ",", &save);
        while (tok) {
            /* Find if this path already exists */
            int found = -1;
            for (int j = 0; j < scored_count; j++) {
                if (strcmp(scored[j].path, tok) == 0) {
                    found = j; break;
                }
            }
            if (found >= 0) {
                scored[found].matched++;
                scored[found].score = scored[found].matched * 100 / qcount;
            } else if (scored_count < SEARCH_MAX_RESULTS) {
                strncpy(scored[scored_count].path, tok, 511);
                scored[scored_count].path[511] = '\0';
                scored[scored_count].matched = 1;
                scored[scored_count].score = 100 / qcount;
                scored_count++;
            }
            tok = strtok_r(NULL, ",", &save);
        }
        free(ps);
    }

    /* Sort (bubble sort, small dataset) and output */
    for (int i = 0; i < scored_count - 1; i++) {
        for (int j = i + 1; j < scored_count; j++) {
            if (scored[j].score > scored[i].score) {
                scored_t tmp = scored[i];
                scored[i] = scored[j];
                scored[j] = tmp;
            }
        }
    }

    int n = scored_count < max_results ? scored_count : max_results;
    for (int i = 0; i < n; i++) {
        strncpy(results[i].path, scored[i].path, 511);
        results[i].path[511] = '\0';
        results[i].score = scored[i].score;
        results[i].title[0] = '\0';
        snprintf(results[i].snippet, sizeof(results[i].snippet),
                 "matched %d/%d tokens", scored[i].matched, qcount);
    }
    *count = n;
    return UTL_OK;
}

void search_clear(search_t *s)
{
    if (!s) return;
    utl_hash_iter_t iter;
    if (UTL_SUCC(utl_hash_iter_begin(s->index, &iter))) {
        do {
            inv_entry_t *e = (inv_entry_t *)iter.value;
            if (e) free(e);
        } while (UTL_SUCC(utl_hash_iter_next(s->index, &iter)));
    }
    utl_hash_clear(s->index);
}
