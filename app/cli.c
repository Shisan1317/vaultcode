/**
 * @file    cli.c
 * @brief   VaultCode CLI — command-line note management tool
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * Usage:
 *   vault-cli --vault ~/my-vault create notes/hello.md "content"
 *   vault-cli --vault ~/my-vault list
 *   vault-cli --vault ~/my-vault search "keywords"
 *   vault-cli --vault ~/my-vault tags
 *   vault-cli --vault ~/my-vault backlinks notes/hello.md
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vault.h"

static void print_note(note_t *n)
{
    printf("=== %s ===\n", n->path);
    printf("Title: %s\n", n->title);
    if (n->date[0])  printf("Date:  %s\n", n->date);
    printf("Tags:  [");
    for (int i = 0; i < n->tag_count; i++)
        printf("%s#%s", i > 0 ? " " : "", n->tags[i]);
    printf("]\n");
    printf("Links: [");
    for (int i = 0; i < n->link_count; i++)
        printf("%s[[%s]]", i > 0 ? " " : "", n->links[i]);
    printf("]\n");
    if (n->backlink_count > 0) {
        printf("Backlinks: [");
        for (int i = 0; i < n->backlink_count; i++)
            printf("%s%s", i > 0 ? " " : "", n->backlinks[i]);
        printf("]\n");
    }
    printf("Size: %zu bytes\n\n", n->content_size);
}

int main(int argc, char *argv[])
{
    if (argc < 3 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s --vault PATH <command> [args...]\n", argv[0]);
        printf("Commands: create PATH CONTENT | list [FOLDER] | get PATH | delete PATH | search QUERY | tags | backlinks PATH\n");
        return 1;
    }

    const char *vault_path = NULL;
    const char *cmd = NULL;
    const char *arg1 = NULL;
    const char *arg2 = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vault") == 0 && i + 1 < argc)
            vault_path = argv[++i];
        else if (!cmd) cmd = argv[i];
        else if (!arg1) arg1 = argv[i];
        else if (!arg2) arg2 = argv[i];
    }

    if (!vault_path || !cmd) {
        fprintf(stderr, "Usage: %s --vault PATH <command>\n", argv[0]);
        return 1;
    }

    vault_t *v = NULL;
    if (UTL_FAIL(vault_open(vault_path, &v))) {
        fprintf(stderr, "Failed to open vault: %s\n", vault_path);
        return 1;
    }

    if (strcmp(cmd, "create") == 0 && arg1 && arg2) {
        utl_err_t r = vault_note_create(v, arg1, arg2);
        printf("%s note: %s\n", UTL_SUCC(r) ? "Created" : "Failed", arg1);
    }
    else if (strcmp(cmd, "get") == 0 && arg1) {
        note_t n;
        if (UTL_SUCC(vault_note_get(v, arg1, &n))) {
            print_note(&n); note_free(&n);
        } else printf("Not found: %s\n", arg1);
    }
    else if (strcmp(cmd, "list") == 0) {
        vault_entry_t entries[256];
        int count = 0;
        vault_note_list(v, arg1 ? arg1 : "", entries, 256, &count);
        printf("%d notes:\n", count);
        for (int i = 0; i < count; i++)
            printf("  %-40s  %-30s  [%d tags]\n", entries[i].path, entries[i].title, entries[i].tag_count);
    }
    else if (strcmp(cmd, "delete") == 0 && arg1) {
        utl_err_t r = vault_note_delete(v, arg1);
        printf("%s delete: %s\n", UTL_SUCC(r) ? "Deleted" : "Failed", arg1);
    }
    else if (strcmp(cmd, "search") == 0 && arg1) {
        search_result_t results[32];
        int count = 0;
        vault_search(v, arg1, results, 32, &count);
        printf("Search results for '%s': %d found\n", arg1, count);
        for (int i = 0; i < count; i++)
            printf("  %-40s  score:%d  %s\n", results[i].path, results[i].score, results[i].snippet);
    }
    else if (strcmp(cmd, "tags") == 0) {
        vault_tag_t tags[256];
        int count = 0;
        vault_tags(v, tags, 256, &count);
        printf("%d tags:\n", count);
        for (int i = 0; i < count; i++)
            printf("  #%-30s  %d notes\n", tags[i].name, tags[i].count);
    }
    else if (strcmp(cmd, "backlinks") == 0 && arg1) {
        vault_entry_t entries[256];
        int count = 0;
        vault_backlinks(v, arg1, entries, 256, &count);
        printf("Backlinks to '%s': %d\n", arg1, count);
        for (int i = 0; i < count; i++)
            printf("  %-40s  %s\n", entries[i].path, entries[i].title);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        vault_close(v);
        return 1;
    }

    vault_close(v);
    return 0;
}
