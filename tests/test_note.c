#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "note.h"
#include "test_harness.h"

static char note_capacity_raw[NOTE_MAX_CONTENT + 2];

bool note_filename_to_title_basic(void)
{
    char title[NOTE_MAX_TITLE] = {0};

    note_filename_to_title("folder/example.md", title, sizeof(title));

    TEST_ASSERT_STREQ("example", title);
    return true;
}

bool note_extract_links_basic(void)
{
    note_t note = {0};
    const char *content = "[[alpha]] and [[beta|display text]]";

    utl_err_t result = note_extract_links(content, &note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_EQ_INT(2, note.link_count);
    TEST_ASSERT_STREQ("alpha", note.links[0]);
    TEST_ASSERT_STREQ("beta", note.links[1]);
    return true;
}

bool note_extract_single_body_tag(void)
{
    note_t note = {0};
    const char *content = "body text #alpha end";

    utl_err_t result = note_extract_tags(content, &note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_EQ_INT(1, note.tag_count);
    TEST_ASSERT_STREQ("alpha", note.tags[0]);
    return true;
}

bool note_parse_copies_content(void)
{
    note_t note = {0};
    char raw[] = "plain content";
    const char *expected = "plain content";
    size_t raw_size = strlen(raw);

    utl_err_t result = note_parse(
        "virtual/example.md", raw, raw_size, &note);

    bool content_not_null = note.content != NULL;
    bool content_is_copy = note.content != raw;
    bool content_matches =
        note.content != NULL && strcmp(expected, note.content) == 0;
    raw[0] = 'P';
    bool content_is_independent =
        note.content != NULL && strcmp(expected, note.content) == 0;
    size_t content_size = note.content_size;

    note_free(&note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT(content_not_null);
    TEST_ASSERT(content_is_copy);
    TEST_ASSERT(content_matches);
    TEST_ASSERT(content_is_independent);
    TEST_ASSERT_EQ_SIZE(raw_size, content_size);
    TEST_ASSERT_NULL(note.content);
    return true;
}

bool note_parse_frontmatter_date(void)
{
    note_t note = {0};
    const char *raw =
        "---\n"
        "date: 2026-07-23\n"
        "---\n"
        "body\n";

    utl_err_t result = note_parse(
        "virtual/frontmatter.md", raw, strlen(raw), &note);

    note_free(&note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_STREQ("2026-07-23", note.date);
    return true;
}

bool note_parse_rejects_null_arguments(void)
{
    note_t path_null_note = {0};
    note_t content_null_note = {0};
    const char *raw = "body";

    utl_err_t path_null_result = note_parse(
        NULL, raw, strlen(raw), &path_null_note);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, path_null_result);

    utl_err_t content_null_result = note_parse(
        "virtual/example.md", NULL, 0, &content_null_note);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, content_null_result);

    utl_err_t output_null_result = note_parse(
        "virtual/example.md", raw, strlen(raw), NULL);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, output_null_result);

    return true;
}

bool note_parse_rejects_oversized_content(void)
{
    note_t note = {0};
    memset(note_capacity_raw, 0, sizeof(note_capacity_raw));

    utl_err_t result = note_parse(
        "virtual/oversized.md",
        note_capacity_raw,
        NOTE_MAX_CONTENT + 1,
        &note);
    TEST_ASSERT_EQ_INT(UTL_ERR_FULL, result);

    return true;
}

bool note_extract_links_rejects_null_arguments(void)
{
    note_t content_null_note = {0};

    utl_err_t content_null_result = note_extract_links(
        NULL, &content_null_note);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, content_null_result);

    utl_err_t output_null_result = note_extract_links(
        "[[alpha]]", NULL);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, output_null_result);

    return true;
}

bool note_extract_tags_rejects_null_arguments(void)
{
    note_t content_null_note = {0};

    utl_err_t content_null_result = note_extract_tags(
        NULL, &content_null_note);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, content_null_result);

    utl_err_t output_null_result = note_extract_tags(
        "#alpha", NULL);
    TEST_ASSERT_EQ_INT(UTL_ERR_NULL, output_null_result);

    return true;
}

bool note_free_repeated_call_is_safe(void)
{
    note_t note = {0};
    note.content = malloc(1);
    TEST_ASSERT_NOT_NULL(note.content);

    note_free(&note);
    TEST_ASSERT_NULL(note.content);

    note_free(&note);
    TEST_ASSERT_NULL(note.content);

    return true;
}

bool note_parse_accepts_max_content_size(void)
{
    note_t note = {0};
    memset(note_capacity_raw, 'x', NOTE_MAX_CONTENT);
    note_capacity_raw[NOTE_MAX_CONTENT] = '\0';

    utl_err_t result = note_parse(
        "virtual/max-content.md",
        note_capacity_raw,
        NOTE_MAX_CONTENT,
        &note);

    bool content_not_null = note.content != NULL;
    size_t content_size = note.content_size;
    char first_char = content_not_null ? note.content[0] : '\0';
    char last_char =
        content_not_null ? note.content[NOTE_MAX_CONTENT - 1] : '\0';
    char terminator =
        content_not_null ? note.content[NOTE_MAX_CONTENT] : 'x';

    note_free(&note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT(content_not_null);
    TEST_ASSERT_EQ_SIZE(NOTE_MAX_CONTENT, content_size);
    TEST_ASSERT_EQ_INT('x', first_char);
    TEST_ASSERT_EQ_INT('x', last_char);
    TEST_ASSERT_EQ_INT('\0', terminator);
    TEST_ASSERT_NULL(note.content);
    return true;
}

bool note_extract_links_stops_at_capacity(void)
{
    note_t note = {0};
    char content[(NOTE_MAX_LINKS + 1) * 16 + 1] = {0};
    size_t used = 0;

    for (int i = 0; i < NOTE_MAX_LINKS + 1; ++i) {
        size_t remaining = sizeof(content) - used;
        int written = snprintf(
            content + used, remaining, "[[link%03d]] ", i);
        TEST_ASSERT(written >= 0);
        TEST_ASSERT((size_t)written < remaining);
        used += (size_t)written;
    }

    utl_err_t result = note_extract_links(content, &note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_EQ_INT(NOTE_MAX_LINKS, note.link_count);
    TEST_ASSERT_STREQ("link000", note.links[0]);
    TEST_ASSERT_STREQ("link127", note.links[NOTE_MAX_LINKS - 1]);
    return true;
}

bool note_extract_tags_stops_at_capacity(void)
{
    note_t note = {0};
    char content[(NOTE_MAX_TAGS + 1) * 16 + 1] = {0};
    size_t used = 0;

    for (int i = 0; i < NOTE_MAX_TAGS + 1; ++i) {
        size_t remaining = sizeof(content) - used;
        int written = snprintf(
            content + used, remaining, "#tag%03d ", i);
        TEST_ASSERT(written >= 0);
        TEST_ASSERT((size_t)written < remaining);
        used += (size_t)written;
    }

    utl_err_t result = note_extract_tags(content, &note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_EQ_INT(NOTE_MAX_TAGS, note.tag_count);
    TEST_ASSERT_STREQ("tag000", note.tags[0]);
    TEST_ASSERT_STREQ("tag063", note.tags[NOTE_MAX_TAGS - 1]);
    return true;
}

bool note_extract_body_tag_length_boundary(void)
{
    note_t max_length_note = {0};
    note_t oversized_note = {0};
    char max_length_tag[NOTE_TAG_MAX_LEN + 2] = {0};
    char oversized_tag[NOTE_TAG_MAX_LEN + 2] = {0};

    max_length_tag[0] = '#';
    memset(max_length_tag + 1, 'a', NOTE_TAG_MAX_LEN - 1);
    max_length_tag[NOTE_TAG_MAX_LEN] = '\0';

    utl_err_t max_length_result = note_extract_tags(
        max_length_tag, &max_length_note);
    TEST_ASSERT_EQ_INT(UTL_OK, max_length_result);
    TEST_ASSERT_EQ_INT(1, max_length_note.tag_count);
    TEST_ASSERT_EQ_SIZE(
        NOTE_TAG_MAX_LEN - 1, strlen(max_length_note.tags[0]));

    oversized_tag[0] = '#';
    memset(oversized_tag + 1, 'a', NOTE_TAG_MAX_LEN);
    oversized_tag[NOTE_TAG_MAX_LEN + 1] = '\0';

    utl_err_t oversized_result = note_extract_tags(
        oversized_tag, &oversized_note);
    TEST_ASSERT_EQ_INT(UTL_OK, oversized_result);
    TEST_ASSERT_EQ_INT(0, oversized_note.tag_count);
    return true;
}

bool note_extract_links_ignores_unclosed_link(void)
{
    note_t note = {0};
    const char *content = "prefix [[alpha";

    utl_err_t result = note_extract_links(content, &note);

    TEST_ASSERT_EQ_INT(UTL_OK, result);
    TEST_ASSERT_EQ_INT(0, note.link_count);
    return true;
}
