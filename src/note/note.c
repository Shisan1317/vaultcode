/**
 * @file    note.c
 * @brief   Note parser implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>

#include "note.h"
#include "os_file.h"

/* ---- Helpers ---- */

static char *prv_strndup(const char *s, size_t n)
{
    char *d = (char *)malloc(n + 1);
    if (d) { memcpy(d, s, n); d[n] = '\0'; }
    return d;
}

static bool prv_is_word_boundary(const char *s, int idx)
{
    if (idx == 0) return true;
    return !isalnum((unsigned char)s[idx - 1]) && s[idx - 1] != '_';
}

/* ---- Frontmatter parsing ---- */

static void prv_parse_frontmatter(const char *content, note_t *note)
{
    /* YAML frontmatter is between --- markers */
    if (strncmp(content, "---\n", 4) != 0) return;

    const char *end = strstr(content + 4, "\n---\n");
    if (!end) end = strstr(content + 4, "\n---");
    if (!end) return;

    size_t fm_len = (size_t)(end - content - 4);
    const char *fm = content + 4;

    /* Parse line by line */
    const char *line_start = fm;
    for (size_t i = 0; i <= fm_len; i++) {
        if (fm[i] == '\n' || i == fm_len) {
            size_t line_len = (size_t)(&fm[i] - line_start);
            if (line_len > 0) {
                /* Look for key: value */
                const char *colon = memchr(line_start, ':', line_len);
                if (colon) {
                    size_t key_len = (size_t)(colon - line_start);
                    const char *val = colon + 1;
                    while (*val == ' ' || *val == '\t') val++;
                    size_t val_len = line_len - (size_t)(val - line_start);

                    if (key_len == 4 && memcmp(line_start, "date", 4) == 0) {
                        size_t n = val_len < 31 ? val_len : 31;
                        memcpy(note->date, val, n);
                        note->date[n] = '\0';
                    } else if (key_len == 4 && memcmp(line_start, "tags", 4) == 0) {
                        size_t n = val_len < 511 ? val_len : 511;
                        memcpy(note->tags_str, val, n);
                        note->tags_str[n] = '\0';
                    }
                }
            }
            line_start = &fm[i + 1];
        }
    }
}

/* ---- Wikilink extraction: [[target]] or [[target|alias]] ---- */

utl_err_t note_extract_links(const char *content, note_t *note)
{
    if (!content || !note) return UTL_ERR_NULL;

    const char *p = content;
    note->link_count = 0;

    while (*p && note->link_count < NOTE_MAX_LINKS) {
        const char *open = strstr(p, "[[");
        if (!open) break;

        const char *close = strstr(open + 2, "]]");
        if (!close) break;

        const char *target_start = open + 2;
        size_t target_len = (size_t)(close - target_start);

        /* Handle alias [[target|alias]] */
        const char *pipe = memchr(target_start, '|', target_len);
        if (pipe) target_len = (size_t)(pipe - target_start);

        if (target_len > 0 && target_len < NOTE_MAX_PATH) {
            memcpy(note->links[note->link_count], target_start, target_len);
            note->links[note->link_count][target_len] = '\0';
            note->link_count++;
        }

        p = close + 2;
    }
    return UTL_OK;
}

/* ---- Tag extraction: #tagname ---- */

utl_err_t note_extract_tags(const char *content, note_t *note)
{
    if (!content || !note) return UTL_ERR_NULL;

    const char *p = content;
    note->tag_count = 0;

    while (*p) {
        if (*p == '#' && prv_is_word_boundary(p, (int)(p - content))) {
            /* Exclude # followed by space/digit/special chars */
            if (isalpha((unsigned char)p[1]) || p[1] == '_') {
                const char *start = p + 1;
                const char *end = start;
                while (isalnum((unsigned char)*end) || *end == '_' ||
                       *end == '-' || *end == '/') end++;

                size_t tag_len = (size_t)(end - start);
                if (tag_len > 1 && tag_len < NOTE_TAG_MAX_LEN &&
                    note->tag_count < NOTE_MAX_TAGS) {
                    memcpy(note->tags[note->tag_count], start, tag_len);
                    note->tags[note->tag_count][tag_len] = '\0';
                    note->tag_count++;
                }
                p = end;
                continue;
            }
        }
        p++;
    }

    /* Also process tags field from frontmatter (bracket format: [tag1, tag2]) */
    if (note->tags_str[0]) {
        const char *ts = note->tags_str;
        while (*ts) {
            /* Skip whitespace and commas */
            while (*ts == ' ' || *ts == '\t' || *ts == ',' || *ts == '[' || *ts == ']' || *ts == '"' || *ts == '\'')
                ts++;
            if (!*ts) break;

            const char *te = ts;
            while (*te && *te != ' ' && *te != '\t' && *te != ',' &&
                   *te != '[' && *te != ']' && *te != '"' && *te != '\'')
                te++;

            size_t tlen = (size_t)(te - ts);
            if (tlen > 0 && tlen < NOTE_TAG_MAX_LEN &&
                note->tag_count < NOTE_MAX_TAGS) {
                /* Check for duplicates */
                bool dup = false;
                for (int i = 0; i < note->tag_count; i++) {
                    if (strncmp(note->tags[i], ts, tlen) == 0 &&
                        note->tags[i][tlen] == '\0') { dup = true; break; }
                }
                if (!dup) {
                    memcpy(note->tags[note->tag_count], ts, tlen);
                    note->tags[note->tag_count][tlen] = '\0';
                    note->tag_count++;
                }
            }
            ts = te;
        }
    }

    return UTL_OK;
}

/* ---- Title extraction: first # heading ---- */

static void prv_extract_title(const char *content, note_t *note)
{
    const char *p = content;
    while (*p) {
        /* Skip frontmatter region */
        if (strncmp(p, "---\n", 4) == 0) {
            const char *end = strstr(p + 4, "\n---\n");
            if (end) { p = end + 5; continue; }
        }

        /* Match # heading (H1) */
        if (*p == '#' && (p == content || p[-1] == '\n')) {
            /* Count all # */
            int level = 0;
            while (*p == '#') { level++; p++; }
            if (*p == ' ') {
                p++; /* skip space */
                const char *start = p;
                while (*p && *p != '\n') p++;
                size_t len = (size_t)(p - start);
                while (len > 0 && (start[len-1] == '\r' || start[len-1] == ' '))
                    len--;
                if (len > 0 && len < NOTE_MAX_TITLE) {
                    memcpy(note->title, start, len);
                    note->title[len] = '\0';
                    return;
                }
            }
        }
        p++;
    }
}

/* ---- Path to default title ---- */

void note_filename_to_title(const char *path, char *title, size_t size)
{
    if (!path || !title) return;
    /* Get filename */
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    /* Strip .md extension */
    const char *dot = strrchr(name, '.');
    size_t len = dot ? (size_t)(dot - name) : strlen(name);
    if (len >= size) len = size - 1;
    memcpy(title, name, len);
    title[len] = '\0';
}

/* ---- Main parser ---- */

utl_err_t note_parse(const char *path, const char *raw_content,
                     size_t raw_size, note_t *note)
{
    if (!path || !raw_content || !note) return UTL_ERR_NULL;
    if (raw_size > NOTE_MAX_CONTENT) return UTL_ERR_FULL;

    memset(note, 0, sizeof(note_t));

    /* Copy path */
    size_t plen = strlen(path);
    if (plen >= NOTE_MAX_PATH) plen = NOTE_MAX_PATH - 1;
    memcpy(note->path, path, plen);
    note->path[plen] = '\0';

    /* Copy content */
    note->content = prv_strndup(raw_content, raw_size);
    if (!note->content) return UTL_ERR_MEM;
    note->content_size = raw_size;

    /* frontmatter */
    prv_parse_frontmatter(raw_content, note);

    /* wikilinks */
    note_extract_links(raw_content, note);

    /* tags */
    note_extract_tags(raw_content, note);

    /* title */
    prv_extract_title(raw_content, note);
    if (!note->title[0]) {
        note_filename_to_title(path, note->title, NOTE_MAX_TITLE);
    }

    /* Timestamps: use current time from stat */
    struct stat st;
    if (stat(path, &st) == 0) {
        note->ctime = (int64_t)st.st_ctime;
        note->mtime = (int64_t)st.st_mtime;
    }

    return UTL_OK;
}

void note_free(note_t *note)
{
    if (!note) return;
    free(note->content);
    note->content = NULL;
}
