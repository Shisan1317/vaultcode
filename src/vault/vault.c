/**
 * @file    vault.c
 * @brief   Vault manager implementation
 * @author  RenJiaqi
 * @version 2.0.0
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

#include "vault.h"
#include "os_file.h"
#include "os_mutex.h"
#include "utl_hash.h"
#include "utl_string.h"

struct vault {
    char         root[VAULT_MAX_PATH];
    utl_hash_t  *notes;       /* path -> note_t* */
    utl_hash_t  *tag_index;   /* tag -> path_list string */
    search_t    *search;
    os_mutex_t  *mutex;
};

/* ---- Build full disk path ---- */
static void prv_full_path(vault_t *v, const char *rel, char *abs, size_t sz)
{
    snprintf(abs, sz, "%s/%s", v->root, rel);
}

/* ---- Ensure parent directory exists ---- */
static utl_err_t prv_ensure_parent(vault_t *v, const char *rel_path)
{
    char parent[VAULT_MAX_PATH];
    utl_str_copy(parent, rel_path, sizeof(parent));
    char *slash = strrchr(parent, '/');
    if (!slash) return UTL_OK;  /* Root directory, no need to create */

    *slash = '\0';
    char abs[VAULT_MAX_PATH * 2];
    snprintf(abs, sizeof(abs), "%s/%s", v->root, parent);

    /* Recursively create directories */
    char cmd[VAULT_MAX_PATH * 3];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", abs);
    system(cmd);
    return UTL_OK;
}

/* ---- Scan directory, build index ---- */
static utl_err_t prv_scan_dir(vault_t *v, const char *rel_base)
{
    char abs_base[VAULT_MAX_PATH * 2];
    prv_full_path(v, rel_base, abs_base, sizeof(abs_base));

    DIR *dir = opendir(abs_base);
    if (!dir) return UTL_ERR_IO;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char child_rel[VAULT_MAX_PATH];
        if (rel_base[0]) snprintf(child_rel, sizeof(child_rel), "%s/%s", rel_base, entry->d_name);
        else             utl_str_copy(child_rel, entry->d_name, sizeof(child_rel));

        char child_abs[VAULT_MAX_PATH * 2];
        prv_full_path(v, child_rel, child_abs, sizeof(child_abs));

        if (entry->d_type == DT_DIR) {
            prv_scan_dir(v, child_rel);
        } else if (entry->d_type == DT_REG &&
                   utl_str_endswith(entry->d_name, ".md")) {
            /* Read file */
            os_file_t *f = NULL;
            if (UTL_FAIL(os_file_open(&f, child_abs, OS_FILE_MODE_READ)))
                continue;

            size_t fsize = 0;
            os_file_size(f, &fsize);
            if (fsize == 0 || fsize > NOTE_MAX_CONTENT) {
                os_file_close(f); continue;
            }

            char *raw = (char *)malloc(fsize + 1);
            if (!raw) { os_file_close(f); continue; }

            size_t rd = 0;
            os_file_read(f, raw, fsize, &rd);
            raw[rd] = '\0';
            os_file_close(f);

            /* Parse */
            note_t *note = (note_t *)calloc(1, sizeof(note_t));
            if (!note) { free(raw); continue; }

            if (UTL_SUCC(note_parse(child_rel, raw, rd, note))) {
                utl_hash_put(v->notes, note->path, note);

                /* Build tag index */
                for (int i = 0; i < note->tag_count; i++) {
                    char *plist = NULL;
                    utl_hash_get(v->tag_index, note->tags[i], (void **)&plist);
                    if (plist) {
                        size_t cur = strlen(plist);
                        snprintf(plist + cur, 1024 - cur, ",%s", child_rel);
                    } else {
                        plist = strdup(child_rel);
                        utl_hash_put(v->tag_index, note->tags[i], plist);
                    }
                }

                /* Search index */
                search_index(v->search, child_rel, raw);
            } else {
                free(note);
            }
            free(raw);
        }
    }
    closedir(dir);
    return UTL_OK;
}

/* ---- Rebuild backlinks ---- */
static void prv_rebuild_backlinks(vault_t *v)
{
    /* Clear all backlinks */
    utl_hash_iter_t it;
    if (UTL_SUCC(utl_hash_iter_begin(v->notes, &it))) {
        do {
            note_t *n = (note_t *)it.value;
            n->backlink_count = 0;
        } while (UTL_SUCC(utl_hash_iter_next(v->notes, &it)));
    }

    /* Scan all links, fill backlinks in reverse */
    if (UTL_SUCC(utl_hash_iter_begin(v->notes, &it))) {
        do {
            note_t *src = (note_t *)it.value;
            for (int i = 0; i < src->link_count; i++) {
                /* Find target note */
                /* Try direct path match */
                note_t *dst = NULL;
                if (UTL_FAIL(utl_hash_get(v->notes, src->links[i], (void **)&dst))) {
                    /* Try append .md extension */
                    char with_md[NOTE_MAX_PATH];
                    size_t link_len = strlen(src->links[i]);
                    if (link_len > sizeof(with_md) - sizeof(".md"))
                        continue;
                    memcpy(with_md, src->links[i], link_len);
                    memcpy(with_md + link_len, ".md", sizeof(".md"));
                    if (UTL_FAIL(utl_hash_get(v->notes, with_md, (void **)&dst)))
                        continue;
                }
                if (dst && dst->backlink_count < NOTE_MAX_BACKLINKS) {
                    strncpy(dst->backlinks[dst->backlink_count], src->path,
                            NOTE_MAX_PATH - 1);
                    dst->backlink_count++;
                }
            }
        } while (UTL_SUCC(utl_hash_iter_next(v->notes, &it)));
    }
}

/* ---- Public API ---- */

utl_err_t vault_open(const char *root_path, vault_t **vault)
{
    if (!root_path || !vault) return UTL_ERR_NULL;

    vault_t *v = (vault_t *)calloc(1, sizeof(vault_t));
    if (!v) return UTL_ERR_MEM;

    utl_str_copy(v->root, root_path, sizeof(v->root));
    mkdir(root_path, 0755);

    /* Create hash tables (key=path string) */
    utl_hash_cfg_t cfg = { .bucket_count = 503, .key_type = UTL_HASH_KEY_STR,
                           .thread_safe = false, .max_size = 0 };
    utl_hash_create(&cfg, &v->notes);
    utl_hash_create(&cfg, &v->tag_index);

    os_mutex_create(&v->mutex);
    search_create(&v->search);

    /* Scan directory */
    prv_scan_dir(v, "");
    prv_rebuild_backlinks(v);

    *vault = v;
    return UTL_OK;
}

void vault_close(vault_t *vault)
{
    if (!vault) return;
    /* Free all notes */
    utl_hash_iter_t it;
    if (UTL_SUCC(utl_hash_iter_begin(vault->notes, &it))) {
        do { note_free((note_t *)it.value); free(it.value); }
        while (UTL_SUCC(utl_hash_iter_next(vault->notes, &it)));
    }
    if (UTL_SUCC(utl_hash_iter_begin(vault->tag_index, &it))) {
        do { free(it.value); }
        while (UTL_SUCC(utl_hash_iter_next(vault->tag_index, &it)));
    }
    utl_hash_destroy(vault->notes);
    utl_hash_destroy(vault->tag_index);
    search_destroy(vault->search);
    if (vault->mutex) os_mutex_destroy(vault->mutex);
    free(vault);
}

utl_err_t vault_note_create(vault_t *v, const char *rel_path, const char *content)
{
    if (!v || !rel_path || !content) return UTL_ERR_NULL;

    os_mutex_lock(v->mutex);

    /* Check if already exists */
    void *tmp = NULL;
    if (UTL_SUCC(utl_hash_get(v->notes, rel_path, &tmp))) {
        os_mutex_unlock(v->mutex);
        return UTL_ERR_EXIST;
    }

    /* Ensure parent directory */
    prv_ensure_parent(v, rel_path);

    /* Write file */
    char abs[VAULT_MAX_PATH * 2];
    prv_full_path(v, rel_path, abs, sizeof(abs));

    os_file_t *f = NULL;
    utl_err_t ret = os_file_open(&f, abs, OS_FILE_MODE_WRITE);
    if (UTL_FAIL(ret)) { os_mutex_unlock(v->mutex); return ret; }
    os_file_write(f, content, strlen(content));
    os_file_close(f);

    /* Parse and add to index */
    note_t *note = (note_t *)calloc(1, sizeof(note_t));
    if (!note) { os_mutex_unlock(v->mutex); return UTL_ERR_MEM; }
    ret = note_parse(rel_path, content, strlen(content), note);
    if (UTL_FAIL(ret)) { free(note); os_mutex_unlock(v->mutex); return ret; }

    utl_hash_put(v->notes, (void *)note->path, note);  /* key=path string */
    search_index(v->search, rel_path, content);

    /* Tag index */
    for (int i = 0; i < note->tag_count; i++) {
        char *plist = strdup(rel_path);
        utl_hash_put(v->tag_index, note->tags[i], plist);
    }

    /* Rebuild backlinks (new note may affect existing ones) */
    prv_rebuild_backlinks(v);

    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_note_get(vault_t *v, const char *rel_path, note_t *note)
{
    if (!v || !rel_path || !note) return UTL_ERR_NULL;

    os_mutex_lock(v->mutex);

    note_t *found = NULL;
    utl_err_t ret = utl_hash_get(v->notes, rel_path, (void **)&found);
    if (UTL_FAIL(ret)) { os_mutex_unlock(v->mutex); return ret; }

    /* Shallow copy to user-provided note */
    memcpy(note, found, sizeof(note_t));
    note->content = found->content ? strdup(found->content) : NULL;

    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_note_update(vault_t *v, const char *rel_path, const char *content)
{
    if (!v || !rel_path || !content) return UTL_ERR_NULL;

    os_mutex_lock(v->mutex);

    note_t *old = NULL;
    if (UTL_FAIL(utl_hash_get(v->notes, rel_path, (void **)&old))) {
        os_mutex_unlock(v->mutex);
        return UTL_ERR_NOT_FOUND;
    }

    /* Write file */
    char abs[VAULT_MAX_PATH * 2];
    prv_full_path(v, rel_path, abs, sizeof(abs));
    os_file_t *f = NULL;
    os_file_open(&f, abs, OS_FILE_MODE_WRITE);
    if (f) { os_file_write(f, content, strlen(content)); os_file_close(f); }

    /* Re-parse */
    note_t fresh;
    memset(&fresh, 0, sizeof(fresh));
    note_parse(rel_path, content, strlen(content), &fresh);

    /* Update note in index (free old content, preserve new) */
    free(old->content);
    memcpy(old, &fresh, sizeof(note_t));
    old->content = fresh.content;  /* Take ownership */

    search_remove(v->search, rel_path);
    search_index(v->search, rel_path, content);
    prv_rebuild_backlinks(v);

    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_note_delete(vault_t *v, const char *rel_path)
{
    if (!v || !rel_path) return UTL_ERR_NULL;
    os_mutex_lock(v->mutex);

    note_t *found = NULL;
    if (UTL_FAIL(utl_hash_get(v->notes, rel_path, (void **)&found))) {
        os_mutex_unlock(v->mutex); return UTL_ERR_NOT_FOUND;
    }

    char abs[VAULT_MAX_PATH * 2];
    prv_full_path(v, rel_path, abs, sizeof(abs));
    unlink(abs);

    utl_err_t remove_result = utl_hash_remove(v->notes, rel_path);
    if (UTL_FAIL(remove_result)) {
        os_mutex_unlock(v->mutex);
        return remove_result;
    }

    note_free(found);
    free(found);
    search_remove(v->search, rel_path);
    prv_rebuild_backlinks(v);

    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_note_list(vault_t *v, const char *folder,
                          vault_entry_t *entries, int max, int *count)
{
    if (!v || !entries || !count) return UTL_ERR_NULL;
    *count = 0;

    os_mutex_lock(v->mutex);
    utl_hash_iter_t it;
    if (UTL_SUCC(utl_hash_iter_begin(v->notes, &it))) {
        do {
            note_t *n = (note_t *)it.value;
            if (folder && folder[0] &&
                !utl_str_startswith(n->path, folder)) continue;

            if (*count < max) {
                utl_str_copy(entries[*count].path, n->path, 512);
                utl_str_copy(entries[*count].title, n->title, 256);
                entries[*count].tag_count = n->tag_count;
                entries[*count].mtime = n->mtime;
                (*count)++;
            }
        } while (UTL_SUCC(utl_hash_iter_next(v->notes, &it)));
    }
    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_search(vault_t *v, const char *query,
                       search_result_t *results, int max, int *count)
{
    if (!v || !query || !results || !count) return UTL_ERR_NULL;
    os_mutex_lock(v->mutex);
    utl_err_t ret = search_query(v->search, query, results, max, count);
    os_mutex_unlock(v->mutex);
    return ret;
}

utl_err_t vault_backlinks(vault_t *v, const char *rel_path,
                          vault_entry_t *entries, int max, int *count)
{
    if (!v || !rel_path || !entries || !count) return UTL_ERR_NULL;
    *count = 0;

    os_mutex_lock(v->mutex);
    note_t *found = NULL;
    if (UTL_FAIL(utl_hash_get(v->notes, rel_path, (void **)&found))) {
        os_mutex_unlock(v->mutex); return UTL_ERR_NOT_FOUND;
    }

    for (int i = 0; i < found->backlink_count && i < max; i++) {
        note_t *src = NULL;
        if (UTL_SUCC(utl_hash_get(v->notes, found->backlinks[i], (void **)&src))) {
            utl_str_copy(entries[i].path, src->path, 512);
            utl_str_copy(entries[i].title, src->title, 256);
            (*count)++;
        }
    }
    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_tags(vault_t *v, vault_tag_t *tags, int max, int *count)
{
    if (!v || !tags || !count) return UTL_ERR_NULL;
    *count = 0;

    os_mutex_lock(v->mutex);
    utl_hash_iter_t it;
    if (UTL_SUCC(utl_hash_iter_begin(v->tag_index, &it))) {
        do {
            if (*count < max) {
                utl_str_copy(tags[*count].name, (const char *)it.key, 64);
                /* Count notes */
                const char *plist = (const char *)it.value;
                int n = 1;
                for (const char *p = plist; *p; p++)
                    if (*p == ',') n++;
                tags[*count].count = n;
                (*count)++;
            }
        } while (UTL_SUCC(utl_hash_iter_next(v->tag_index, &it)));
    }
    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_notes_by_tag(vault_t *v, const char *tag,
                             vault_entry_t *entries, int max, int *count)
{
    if (!v || !tag || !entries || !count) return UTL_ERR_NULL;
    *count = 0;

    os_mutex_lock(v->mutex);
    char *plist = NULL;
    if (UTL_FAIL(utl_hash_get(v->tag_index, tag, (void **)&plist))) {
        os_mutex_unlock(v->mutex); return UTL_OK;
    }

    char *ps = strdup(plist);
    char *save = NULL;
    char *tok = strtok_r(ps, ",", &save);
    while (tok && *count < max) {
        note_t *n = NULL;
        if (UTL_SUCC(utl_hash_get(v->notes, tok, (void **)&n))) {
            utl_str_copy(entries[*count].path, n->path, 512);
            utl_str_copy(entries[*count].title, n->title, 256);
            (*count)++;
        }
        tok = strtok_r(NULL, ",", &save);
    }
    free(ps);
    os_mutex_unlock(v->mutex);
    return UTL_OK;
}

utl_err_t vault_folder_create(vault_t *v, const char *rel_path)
{
    if (!v || !rel_path) return UTL_ERR_NULL;
    char abs[VAULT_MAX_PATH * 2];
    prv_full_path(v, rel_path, abs, sizeof(abs));
    mkdir(abs, 0755);
    return UTL_OK;
}

utl_err_t vault_folder_list(vault_t *v, const char *folder,
                            char names[][256], int max, int *count)
{
    if (!v || !names || !count) return UTL_ERR_NULL;
    *count = 0;

    char abs[VAULT_MAX_PATH * 2];
    prv_full_path(v, folder ? folder : "", abs, sizeof(abs));

    DIR *dir = opendir(abs);
    if (!dir) return UTL_ERR_IO;

    struct dirent *e;
    while ((e = readdir(dir)) != NULL && *count < max) {
        if (e->d_name[0] == '.') continue;
        if (e->d_type == DT_DIR) {
            utl_str_copy(names[*count], e->d_name, 256);
            (*count)++;
        }
    }
    closedir(dir);
    return UTL_OK;
}

utl_err_t vault_info(vault_t *v, int *note_count, int *tag_count)
{
    if (!v || !note_count || !tag_count) return UTL_ERR_NULL;
    os_mutex_lock(v->mutex);
    size_t nc = 0, tc = 0;
    utl_hash_size(v->notes, &nc);
    utl_hash_size(v->tag_index, &tc);
    *note_count = (int)nc;
    *tag_count = (int)tc;
    os_mutex_unlock(v->mutex);
    return UTL_OK;
}
