/**
 * @file    vault.h
 * @brief   Obsidian-Style Vault Manager — Note vault manager
 * @author  RenJiaqi
 * @version 2.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Manages all .md files under a directory
 *   - Maintains in-memory indexes: path->note, backlink graph, tag->notes mapping
 *   - All CRUD operations auto-update indexes
 *   - Thread-safe
 */

#ifndef VAULT_H
#define VAULT_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"
#include "note.h"
#include "utl_search.h"

UTL_EXTERN_C_BEGIN

#define VAULT_MAX_PATH   1024

typedef struct vault vault_t;

/* List entry (simplified) */
typedef struct {
    char  path[512];
    char  title[256];
    int   tag_count;
    int64_t mtime;
} vault_entry_t;

/* Tag statistics */
typedef struct {
    char  name[64];
    int   count;
} vault_tag_t;

#define VAULT_MAX_ENTRIES  2048
#define VAULT_MAX_TAGS     256

/* ---- API ---- */

/**
 * @brief Open a vault rooted at a directory.
 * @param[out] vault Caller-owned handle released with vault_close.
 */
utl_err_t vault_open(const char *root_path, vault_t **vault);

/// @brief Release the vault handle without deleting its directory or note files.
void      vault_close(vault_t *vault);

/* CRUD */

/// @brief Create and index a note; an identical existing path returns UTL_ERR_EXIST.
utl_err_t vault_note_create(vault_t *v, const char *rel_path, const char *content);

/// @brief Copy a note to caller storage; release its copied content with note_free.
utl_err_t vault_note_get(vault_t *v, const char *rel_path, note_t *note);
utl_err_t vault_note_update(vault_t *v, const char *rel_path, const char *content);
utl_err_t vault_note_delete(vault_t *v, const char *rel_path);

/* List */

/// @brief List matching notes in unspecified order into caller-provided storage.
utl_err_t vault_note_list(vault_t *v, const char *folder,
                          vault_entry_t *entries, int max, int *count);

/* Search */
utl_err_t vault_search(vault_t *v, const char *query,
                       search_result_t *results, int max, int *count);

/* Backlinks */
utl_err_t vault_backlinks(vault_t *v, const char *rel_path,
                          vault_entry_t *entries, int max, int *count);

/* Tags */
utl_err_t vault_tags(vault_t *v, vault_tag_t *tags, int max, int *count);
utl_err_t vault_notes_by_tag(vault_t *v, const char *tag,
                             vault_entry_t *entries, int max, int *count);

/* Folders */
utl_err_t vault_folder_create(vault_t *v, const char *rel_path);
utl_err_t vault_folder_list(vault_t *v, const char *folder,
                            char names[][256], int max, int *count);

/* Info */
utl_err_t vault_info(vault_t *v, int *note_count, int *tag_count);

UTL_EXTERN_C_END

#endif /* VAULT_H */
