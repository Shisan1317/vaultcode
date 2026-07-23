#define _DEFAULT_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_harness.h"
#include "vault.h"

typedef struct {
    char root[VAULT_MAX_PATH];
} vault_test_fixture_t;

static bool prv_fixture_create(vault_test_fixture_t *fixture)
{
    const char *temp_base = getenv("TMPDIR");
    if (!temp_base || !temp_base[0]) {
#ifdef P_tmpdir
        temp_base = P_tmpdir;
#else
        return false;
#endif
    }

    int written = snprintf(
        fixture->root,
        sizeof(fixture->root),
        "%s/vaultcode-tests-XXXXXX",
        temp_base);
    if (written < 0 || (size_t)written >= sizeof(fixture->root)) {
        fixture->root[0] = '\0';
        return false;
    }

    return mkdtemp(fixture->root) != NULL;
}

static bool prv_fixture_remove_file(
    const vault_test_fixture_t *fixture,
    const char *rel_path)
{
    char path[VAULT_MAX_PATH * 2];
    int written = snprintf(
        path, sizeof(path), "%s/%s", fixture->root, rel_path);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return false;
    }

    if (unlink(path) == 0) {
        return true;
    }
    return errno == ENOENT;
}

static bool prv_fixture_remove_root(vault_test_fixture_t *fixture)
{
    if (!fixture->root[0]) {
        return true;
    }

    bool removed = rmdir(fixture->root) == 0;
    if (removed) {
        fixture->root[0] = '\0';
    }
    return removed;
}

static bool prv_fixture_remove_parent_dirs(
    const vault_test_fixture_t *fixture,
    const char *rel_path)
{
    char parent[NOTE_MAX_PATH];
    int copied = snprintf(parent, sizeof(parent), "%s", rel_path);
    if (copied < 0 || (size_t)copied >= sizeof(parent)) {
        return false;
    }

    char *slash = strrchr(parent, '/');
    while (slash) {
        *slash = '\0';

        char path[VAULT_MAX_PATH * 2];
        int written = snprintf(
            path, sizeof(path), "%s/%s", fixture->root, parent);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            return false;
        }
        if (rmdir(path) != 0 && errno != ENOENT) {
            return false;
        }

        slash = strrchr(parent, '/');
    }
    return true;
}

static bool prv_build_long_link(
    char *link,
    size_t capacity,
    size_t link_len)
{
    const size_t component_len = 120;
    const size_t directory_count = 3;
    const size_t prefix_len =
        directory_count * (component_len + 1);

    if (!link || link_len < prefix_len ||
        link_len >= capacity) {
        return false;
    }

    size_t offset = 0;
    for (size_t i = 0; i < directory_count; ++i) {
        memset(link + offset, (int)('a' + i), component_len);
        offset += component_len;
        link[offset++] = '/';
    }
    memset(link + offset, 'd', link_len - offset);
    link[link_len] = '\0';
    return true;
}

static bool prv_build_wikilink(
    char *content,
    size_t capacity,
    const char *link)
{
    size_t link_len = strlen(link);
    size_t required = sizeof("[[") - 1 + link_len + sizeof("]]");
    if (required > capacity) {
        return false;
    }

    size_t offset = 0;
    memcpy(content + offset, "[[", sizeof("[[") - 1);
    offset += sizeof("[[") - 1;
    memcpy(content + offset, link, link_len);
    offset += link_len;
    memcpy(content + offset, "]]", sizeof("]]"));
    return true;
}

bool vault_open_empty_and_info(void)
{
    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    vault_t *vault = NULL;
    int note_count = -1;
    int tag_count = -1;
    utl_err_t info_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    bool vault_not_null = vault != NULL;
    if (UTL_SUCC(open_result) && vault_not_null) {
        info_result = vault_info(vault, &note_count, &tag_count);
    }

    if (vault) {
        vault_close(vault);
    }
    bool cleanup_succeeded = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT(vault_not_null);
    TEST_ASSERT_EQ_INT(UTL_OK, info_result);
    TEST_ASSERT_EQ_INT(0, note_count);
    TEST_ASSERT_EQ_INT(0, tag_count);
    TEST_ASSERT(cleanup_succeeded);
    return true;
}

bool vault_create_and_get_note(void)
{
    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    const char *rel_path = "alpha.md";
    const char *content = "plain content";
    vault_t *vault = NULL;
    note_t note = {0};
    char actual_path[NOTE_MAX_PATH] = {0};
    char actual_content[32] = {0};
    bool content_not_null = false;
    utl_err_t create_result = UTL_ERR_INTERNAL;
    utl_err_t get_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        create_result = vault_note_create(vault, rel_path, content);
    }
    if (UTL_SUCC(create_result)) {
        get_result = vault_note_get(vault, rel_path, &note);
    }
    if (UTL_SUCC(get_result)) {
        snprintf(actual_path, sizeof(actual_path), "%s", note.path);
        content_not_null = note.content != NULL;
        if (content_not_null) {
            snprintf(
                actual_content, sizeof(actual_content), "%s", note.content);
        }
    }

    note_free(&note);
    if (vault) {
        vault_close(vault);
    }
    bool file_removed = prv_fixture_remove_file(&fixture, rel_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, get_result);
    TEST_ASSERT_STREQ(rel_path, actual_path);
    TEST_ASSERT(content_not_null);
    TEST_ASSERT_STREQ(content, actual_content);
    TEST_ASSERT(file_removed);
    TEST_ASSERT(root_removed);
    return true;
}

bool vault_list_notes_after_create(void)
{
    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    const char *alpha_path = "alpha.md";
    const char *beta_path = "beta.md";
    vault_t *vault = NULL;
    vault_entry_t entries[2] = {0};
    int count = -1;
    utl_err_t alpha_result = UTL_ERR_INTERNAL;
    utl_err_t beta_result = UTL_ERR_INTERNAL;
    utl_err_t list_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        alpha_result = vault_note_create(
            vault, alpha_path, "alpha content");
    }
    if (UTL_SUCC(alpha_result)) {
        beta_result = vault_note_create(
            vault, beta_path, "beta content");
    }
    if (UTL_SUCC(beta_result)) {
        list_result = vault_note_list(vault, "", entries, 2, &count);
    }

    bool found_alpha = false;
    bool found_beta = false;
    for (int i = 0; i < count && i < 2; ++i) {
        if (strcmp(entries[i].path, alpha_path) == 0) {
            found_alpha = true;
        } else if (strcmp(entries[i].path, beta_path) == 0) {
            found_beta = true;
        }
    }

    if (vault) {
        vault_close(vault);
    }
    bool alpha_removed = prv_fixture_remove_file(&fixture, alpha_path);
    bool beta_removed = prv_fixture_remove_file(&fixture, beta_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, alpha_result);
    TEST_ASSERT_EQ_INT(UTL_OK, beta_result);
    TEST_ASSERT_EQ_INT(UTL_OK, list_result);
    TEST_ASSERT_EQ_INT(2, count);
    TEST_ASSERT(found_alpha);
    TEST_ASSERT(found_beta);
    TEST_ASSERT(alpha_removed);
    TEST_ASSERT(beta_removed);
    TEST_ASSERT(root_removed);
    return true;
}

bool vault_rejects_duplicate_note(void)
{
    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    const char *rel_path = "duplicate.md";
    vault_t *vault = NULL;
    utl_err_t first_create_result = UTL_ERR_INTERNAL;
    utl_err_t duplicate_create_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        first_create_result = vault_note_create(
            vault, rel_path, "original content");
    }
    if (UTL_SUCC(first_create_result)) {
        duplicate_create_result = vault_note_create(
            vault, rel_path, "replacement content");
    }

    if (vault) {
        vault_close(vault);
    }
    bool file_removed = prv_fixture_remove_file(&fixture, rel_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, first_create_result);
    TEST_ASSERT_EQ_INT(UTL_ERR_EXIST, duplicate_create_result);
    TEST_ASSERT(file_removed);
    TEST_ASSERT(root_removed);
    return true;
}

bool vault_delete_removes_note_safely(void)
{
    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    char create_path[] = "delete-target.md";
    char delete_path[] = "delete-target.md";
    vault_t *vault = NULL;
    note_t after_delete = {0};
    note_t after_reopen = {0};
    bool first_vault_closed = false;
    bool reopened_vault_closed = false;
    utl_err_t create_result = UTL_ERR_INTERNAL;
    utl_err_t delete_result = UTL_ERR_INTERNAL;
    utl_err_t get_after_delete_result = UTL_ERR_INTERNAL;
    utl_err_t reopen_result = UTL_ERR_INTERNAL;
    utl_err_t get_after_reopen_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        create_result = vault_note_create(
            vault, create_path, "delete content");
    }
    if (UTL_SUCC(create_result)) {
        delete_result = vault_note_delete(vault, delete_path);
    }
    if (UTL_SUCC(delete_result)) {
        get_after_delete_result = vault_note_get(
            vault, delete_path, &after_delete);
    }

    note_free(&after_delete);
    if (vault) {
        vault_close(vault);
        vault = NULL;
        first_vault_closed = true;
    }

    if (UTL_SUCC(delete_result)) {
        reopen_result = vault_open(fixture.root, &vault);
    }
    if (UTL_SUCC(reopen_result) && vault) {
        get_after_reopen_result = vault_note_get(
            vault, delete_path, &after_reopen);
    }

    note_free(&after_reopen);
    if (vault) {
        vault_close(vault);
        reopened_vault_closed = true;
    }
    bool file_removed = prv_fixture_remove_file(
        &fixture, create_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, delete_result);
    TEST_ASSERT_EQ_INT(UTL_ERR_NOT_FOUND, get_after_delete_result);
    TEST_ASSERT(first_vault_closed);
    TEST_ASSERT_EQ_INT(UTL_OK, reopen_result);
    TEST_ASSERT_EQ_INT(UTL_ERR_NOT_FOUND, get_after_reopen_result);
    TEST_ASSERT(reopened_vault_closed);
    TEST_ASSERT(file_removed);
    TEST_ASSERT(root_removed);
    return true;
}

bool vault_backlinks_skip_oversized_extension_key(void)
{
    const size_t link_len = NOTE_MAX_PATH - sizeof(".md") + 1;
    char link[NOTE_MAX_PATH] = {0};
    char collision_path[NOTE_MAX_PATH] = {0};
    char source_content[NOTE_MAX_PATH + 5] = {0};
    bool link_built = prv_build_long_link(
        link, sizeof(link), link_len);
    TEST_ASSERT(link_built);
    memcpy(collision_path, link, link_len);
    memcpy(collision_path + link_len, ".m", sizeof(".m"));
    bool content_built = prv_build_wikilink(
        source_content, sizeof(source_content), link);
    TEST_ASSERT(content_built);

    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    const char *source_path = "oversized-source.md";
    vault_t *vault = NULL;
    note_t collision_note = {0};
    vault_entry_t entries[1] = {0};
    int backlink_count = -1;
    utl_err_t target_create_result = UTL_ERR_INTERNAL;
    utl_err_t source_create_result = UTL_ERR_INTERNAL;
    utl_err_t target_get_result = UTL_ERR_INTERNAL;
    utl_err_t backlinks_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        target_create_result = vault_note_create(
            vault, collision_path, "collision target");
    }
    if (UTL_SUCC(target_create_result)) {
        source_create_result = vault_note_create(
            vault, source_path, source_content);
    }
    if (UTL_SUCC(source_create_result)) {
        target_get_result = vault_note_get(
            vault, collision_path, &collision_note);
    }
    if (UTL_SUCC(target_get_result)) {
        backlinks_result = vault_backlinks(
            vault, collision_path, entries, 1, &backlink_count);
    }

    bool loaded_target_matches =
        UTL_SUCC(target_get_result) &&
        strcmp(collision_note.path, collision_path) == 0;
    note_free(&collision_note);
    if (vault) {
        vault_close(vault);
    }
    bool target_removed = prv_fixture_remove_file(
        &fixture, collision_path);
    bool source_removed = prv_fixture_remove_file(
        &fixture, source_path);
    bool parents_removed = prv_fixture_remove_parent_dirs(
        &fixture, collision_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, target_create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, source_create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, target_get_result);
    TEST_ASSERT(loaded_target_matches);
    TEST_ASSERT_EQ_INT(UTL_OK, backlinks_result);
    TEST_ASSERT_EQ_INT(0, backlink_count);
    TEST_ASSERT(target_removed);
    TEST_ASSERT(source_removed);
    TEST_ASSERT(parents_removed);
    TEST_ASSERT(root_removed);
    return true;
}

bool vault_backlinks_accept_max_extension_key(void)
{
    const size_t link_len = NOTE_MAX_PATH - sizeof(".md");
    char link[NOTE_MAX_PATH] = {0};
    char target_path[NOTE_MAX_PATH] = {0};
    char source_content[NOTE_MAX_PATH + 5] = {0};
    bool link_built = prv_build_long_link(
        link, sizeof(link), link_len);
    TEST_ASSERT(link_built);
    memcpy(target_path, link, link_len);
    memcpy(target_path + link_len, ".md", sizeof(".md"));
    bool content_built = prv_build_wikilink(
        source_content, sizeof(source_content), link);
    TEST_ASSERT(content_built);

    vault_test_fixture_t fixture = {0};
    bool fixture_created = prv_fixture_create(&fixture);
    TEST_ASSERT(fixture_created);

    const char *source_path = "boundary-source.md";
    vault_t *vault = NULL;
    note_t target_note = {0};
    vault_entry_t entries[1] = {0};
    int backlink_count = -1;
    utl_err_t target_create_result = UTL_ERR_INTERNAL;
    utl_err_t source_create_result = UTL_ERR_INTERNAL;
    utl_err_t target_get_result = UTL_ERR_INTERNAL;
    utl_err_t backlinks_result = UTL_ERR_INTERNAL;

    utl_err_t open_result = vault_open(fixture.root, &vault);
    if (UTL_SUCC(open_result) && vault) {
        target_create_result = vault_note_create(
            vault, target_path, "boundary target");
    }
    if (UTL_SUCC(target_create_result)) {
        source_create_result = vault_note_create(
            vault, source_path, source_content);
    }
    if (UTL_SUCC(source_create_result)) {
        target_get_result = vault_note_get(
            vault, target_path, &target_note);
    }
    if (UTL_SUCC(target_get_result)) {
        backlinks_result = vault_backlinks(
            vault, target_path, entries, 1, &backlink_count);
    }

    bool loaded_target_matches =
        UTL_SUCC(target_get_result) &&
        strcmp(target_note.path, target_path) == 0;
    note_free(&target_note);
    if (vault) {
        vault_close(vault);
    }
    bool target_removed = prv_fixture_remove_file(
        &fixture, target_path);
    bool source_removed = prv_fixture_remove_file(
        &fixture, source_path);
    bool parents_removed = prv_fixture_remove_parent_dirs(
        &fixture, target_path);
    bool root_removed = prv_fixture_remove_root(&fixture);

    TEST_ASSERT_EQ_INT(UTL_OK, open_result);
    TEST_ASSERT_EQ_INT(UTL_OK, target_create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, source_create_result);
    TEST_ASSERT_EQ_INT(UTL_OK, target_get_result);
    TEST_ASSERT(loaded_target_matches);
    TEST_ASSERT_EQ_INT(UTL_OK, backlinks_result);
    TEST_ASSERT_EQ_INT(1, backlink_count);
    TEST_ASSERT_STREQ(source_path, entries[0].path);
    TEST_ASSERT(target_removed);
    TEST_ASSERT(source_removed);
    TEST_ASSERT(parents_removed);
    TEST_ASSERT(root_removed);
    return true;
}
