/**
 * @file    server.c
 * @brief   VaultCode Daemon — Obsidian-like note service
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * Usage: vaultd --socket /tmp/vaultd.sock --vault ~/my-vault
 *
 * JSON-RPC 2.0 over Unix Domain Socket
 * One JSON request per line, \n delimited, response is also one-line JSON
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vault.h"
#include "server.h"
#include "utl_string.h"

static vault_t *g_vault = NULL;

/* ---- Minimal JSON builder ---- */

static char *json_ok(const char *id, const char *result_body)
{
    char buf[8192];
    snprintf(buf, sizeof(buf),
             "{\"jsonrpc\":\"2.0\",\"result\":%s,\"id\":\"%s\"}",
             result_body, id);
    return strdup(buf);
}

static char *json_err(const char *id, int code, const char *msg)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
             "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":%d,\"message\":\"%s\"},\"id\":\"%s\"}",
             code, msg, id);
    return strdup(buf);
}

/* ---- Parameter extraction helpers ---- */

static char *param_str(const char *params, const char *key)
{
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *pos = strstr(params, search);
    if (!pos) return NULL;
    pos += strlen(search);
    while (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '"') pos++;
    if (*pos == '\0') return NULL;
    const char *end = strchr(pos, '"');
    if (!end) return NULL;
    size_t len = (size_t)(end - pos);
    char *val = (char *)malloc(len + 1);
    memcpy(val, pos, len); val[len] = '\0';
    return val;
}

/* ---- Method dispatch ---- */

static char *dispatch(const char *method, const char *params, const char *id)
{
    if (!g_vault) {
        /* Initialization: vault.open */
        if (strcmp(method, "vault.open") == 0) {
            char *path = param_str(params, "path");
            if (!path || !path[0]) { free(path); return json_err(id, -1, "missing path"); }

            utl_err_t ret = vault_open(path, &g_vault);
            free(path);
            if (UTL_FAIL(ret)) return json_err(id, ret, "Failed to open vault");

            int nc = 0, tc = 0;
            vault_info(g_vault, &nc, &tc);
            char body[512];
            snprintf(body, sizeof(body), "{\"path\":\"%s\",\"note_count\":%d,\"tag_count\":%d}",
                     g_vault ? "opened" : "null", nc, tc);
            return json_ok(id, body);
        }
        return json_err(id, -1, "Vault not open. Call vault.open first.");
    }

    /* vault.info */
    if (strcmp(method, "vault.info") == 0) {
        int nc = 0, tc = 0;
        vault_info(g_vault, &nc, &tc);
        char body[256];
        snprintf(body, sizeof(body), "{\"note_count\":%d,\"tag_count\":%d}", nc, tc);
        return json_ok(id, body);
    }

    /* note.create */
    if (strcmp(method, "note.create") == 0) {
        char *path = param_str(params, "path");
        char *content = param_str(params, "content");
        if (!path || !content) { free(path); free(content); return json_err(id, -1, "missing params"); }

        /* \n escapes in JSON need unescaping */
        /* Simplified: look up content value directly from params (may contain \n) */
        char *real_content = param_str(params, "content");
        utl_err_t ret = vault_note_create(g_vault, path, content);

        char body[1024];
        if (UTL_SUCC(ret)) {
            note_t note;
            if (UTL_SUCC(vault_note_get(g_vault, path, &note))) {
                snprintf(body, sizeof(body),
                         "{\"path\":\"%s\",\"title\":\"%s\",\"tags\":%d,\"links\":%d,\"size\":%zu}",
                         path, note.title, note.tag_count, note.link_count,
                         note.content_size);
                note_free(&note);
            } else {
                snprintf(body, sizeof(body), "{\"path\":\"%s\"}", path);
            }
        }

        free(path); free(content); free(real_content);
        if (UTL_FAIL(ret)) return json_err(id, ret, "Create failed");
        return json_ok(id, body);
    }

    /* note.get */
    if (strcmp(method, "note.get") == 0) {
        char *path = param_str(params, "path");
        if (!path) return json_err(id, -1, "missing path");

        note_t note;
        utl_err_t ret = vault_note_get(g_vault, path, &note);
        if (UTL_FAIL(ret)) { free(path); return json_err(id, ret, "Not found"); }

        char body[8192];
        /* Escape quotes and newlines in content */
        char *escaped = (char *)malloc(note.content_size * 2 + 1);
        char *ep = escaped;
        for (size_t i = 0; i < note.content_size && note.content[i]; i++) {
            if (note.content[i] == '"')  { *ep++ = '\\'; *ep++ = '"'; }
            else if (note.content[i] == '\n') { *ep++ = '\\'; *ep++ = 'n'; }
            else if (note.content[i] == '\\') { *ep++ = '\\'; *ep++ = '\\'; }
            else *ep++ = note.content[i];
        }
        *ep = '\0';

        /* Build links JSON array */
        char links_json[2048] = "[";
        for (int i = 0; i < note.link_count; i++) {
            char tmp[NOTE_MAX_PATH + 4];
            snprintf(tmp, sizeof(tmp), "%s\"%s\"", i > 0 ? "," : "", note.links[i]);
            strcat(links_json, tmp);
        }
        strcat(links_json, "]");

        char tags_json[2048] = "[";
        for (int i = 0; i < note.tag_count; i++) {
            char tmp[NOTE_TAG_MAX_LEN + 4];
            snprintf(tmp, sizeof(tmp), "%s\"%s\"", i > 0 ? "," : "", note.tags[i]);
            strcat(tags_json, tmp);
        }
        strcat(tags_json, "]");

        char bl_json[4096] = "[";
        for (int i = 0; i < note.backlink_count; i++) {
            char tmp[NOTE_MAX_PATH + 4];
            snprintf(tmp, sizeof(tmp), "%s\"%s\"", i > 0 ? "," : "", note.backlinks[i]);
            strcat(bl_json, tmp);
        }
        strcat(bl_json, "]");

        snprintf(body, sizeof(body),
                 "{\"path\":\"%s\",\"title\":\"%s\",\"content\":\"%s\","
                 "\"tags\":%s,\"links\":%s,\"backlinks\":%s,"
                 "\"ctime\":%lld,\"mtime\":%lld}",
                 note.path, note.title, escaped,
                 tags_json, links_json, bl_json,
                 (long long)note.ctime, (long long)note.mtime);

        free(escaped); free(path);
        note_free(&note);
        return json_ok(id, body);
    }

    /* note.update */
    if (strcmp(method, "note.update") == 0) {
        char *path = param_str(params, "path");
        char *content = param_str(params, "content");
        if (!path || !content) { free(path); free(content); return json_err(id, -1, "missing params"); }
        utl_err_t ret = vault_note_update(g_vault, path, content);
        free(path); free(content);
        if (UTL_FAIL(ret)) return json_err(id, ret, "Update failed");
        return json_ok(id, "{}");
    }

    /* note.delete */
    if (strcmp(method, "note.delete") == 0) {
        char *path = param_str(params, "path");
        if (!path) return json_err(id, -1, "missing path");
        utl_err_t ret = vault_note_delete(g_vault, path);
        free(path);
        if (UTL_FAIL(ret)) return json_err(id, ret, "Delete failed");
        return json_ok(id, "{}");
    }

    /* note.list */
    if (strcmp(method, "note.list") == 0) {
        char *folder = param_str(params, "folder");
        vault_entry_t entries[VAULT_MAX_ENTRIES];
        int count = 0;
        vault_note_list(g_vault, folder ? folder : "", entries, 256, &count);
        free(folder);

        char body[16384];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s{\"path\":\"%s\",\"title\":\"%s\",\"tags\":%d,\"mtime\":%lld}",
                            i > 0 ? "," : "",
                            entries[i].path, entries[i].title,
                            entries[i].tag_count, (long long)entries[i].mtime);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    /* note.search */
    if (strcmp(method, "note.search") == 0) {
        char *query = param_str(params, "query");
        if (!query) return json_err(id, -1, "missing query");

        search_result_t results[SEARCH_MAX_RESULTS];
        int count = 0;
        vault_search(g_vault, query, results, SEARCH_MAX_RESULTS, &count);
        free(query);

        char body[8192];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s{\"path\":\"%s\",\"snippet\":\"%s\",\"score\":%d}",
                            i > 0 ? "," : "",
                            results[i].path, results[i].snippet, results[i].score);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    /* note.backlinks */
    if (strcmp(method, "note.backlinks") == 0) {
        char *path = param_str(params, "path");
        vault_entry_t entries[VAULT_MAX_ENTRIES];
        int count = 0;
        vault_backlinks(g_vault, path ? path : "", entries, 256, &count);
        free(path);

        char body[4096];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s{\"path\":\"%s\",\"title\":\"%s\"}",
                            i > 0 ? "," : "", entries[i].path, entries[i].title);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    /* note.tags */
    if (strcmp(method, "note.tags") == 0) {
        vault_tag_t tags[VAULT_MAX_TAGS];
        int count = 0;
        vault_tags(g_vault, tags, VAULT_MAX_TAGS, &count);

        char body[4096];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s{\"name\":\"%s\",\"count\":%d}",
                            i > 0 ? "," : "", tags[i].name, tags[i].count);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    /* note.by_tag */
    if (strcmp(method, "note.by_tag") == 0) {
        char *tag = param_str(params, "tag");
        vault_entry_t entries[VAULT_MAX_ENTRIES];
        int count = 0;
        vault_notes_by_tag(g_vault, tag ? tag : "", entries, 256, &count);
        free(tag);

        char body[4096];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s{\"path\":\"%s\",\"title\":\"%s\"}",
                            i > 0 ? "," : "", entries[i].path, entries[i].title);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    /* folder.create */
    if (strcmp(method, "folder.create") == 0) {
        char *path = param_str(params, "path");
        if (!path) return json_err(id, -1, "missing path");
        vault_folder_create(g_vault, path);
        free(path);
        return json_ok(id, "{}");
    }

    /* folder.list */
    if (strcmp(method, "folder.list") == 0) {
        char *path = param_str(params, "path");
        char names[256][256];
        int count = 0;
        vault_folder_list(g_vault, path, names, 256, &count);
        free(path);

        char body[4096];
        int off = snprintf(body, sizeof(body), "[");
        for (int i = 0; i < count; i++) {
            off += snprintf(body + off, sizeof(body) - off,
                            "%s\"%s\"", i > 0 ? "," : "", names[i]);
        }
        off += snprintf(body + off, sizeof(body) - off, "]");
        return json_ok(id, body);
    }

    return json_err(id, -32601, "Method not found");
}

/* ---- main ---- */

int main(int argc, char *argv[])
{
    const char *sock_path = "/tmp/vaultd.sock";
    const char *vault_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--socket") == 0 && i + 1 < argc)
            sock_path = argv[++i];
        else if (strcmp(argv[i], "--vault") == 0 && i + 1 < argc)
            vault_path = argv[++i];
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("VaultCode Daemon v1.0\n");
            printf("Usage: %s --socket PATH --vault PATH\n", argv[0]);
            printf("  --socket   Unix socket path (default: /tmp/vaultd.sock)\n");
            printf("  --vault    Vault directory path\n");
            printf("  --help     Show this help\n");
            return 0;
        }
    }

    /* If vault path specified, pre-open it */
    if (vault_path) {
        printf("Opening vault: %s\n", vault_path);
        if (UTL_FAIL(vault_open(vault_path, &g_vault))) {
            fprintf(stderr, "Failed to open vault: %s\n", vault_path);
            return 1;
        }
        int nc, tc;
        vault_info(g_vault, &nc, &tc);
        printf("Vault ready: %d notes, %d tags\n", nc, tc);
    }

    server_t *srv = NULL;
    utl_err_t ret = server_create(sock_path, dispatch, &srv);
    if (UTL_FAIL(ret)) {
        fprintf(stderr, "Failed to create server: %d\n", ret);
        if (g_vault) vault_close(g_vault);
        return 1;
    }

    printf("VaultCode daemon listening on %s\n", sock_path);
    printf("Press Ctrl+C to stop\n");

    server_run(srv);

    server_destroy(srv);
    if (g_vault) vault_close(g_vault);
    return 0;
}
