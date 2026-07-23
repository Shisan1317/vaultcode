/**
 * @file    note.h
 * @brief   Markdown note parser — frontmatter / wikilink / tag extraction
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 *
 * Description:
 *   - Parses standard Markdown .md files
 *   - Extracts YAML frontmatter (key-value pairs between --- delimiters)
 *   - Extracts [[wikilinks]] and [[target|alias]]
 *   - Extracts #tags
 *   - Extracts first # heading as title
 */

#ifndef NOTE_H
#define NOTE_H

#include <stddef.h>
#include "utl_types.h"
#include "utl_err.h"
#include "utl_config.h"

UTL_EXTERN_C_BEGIN

/* Constants */
#define NOTE_MAX_PATH       512
#define NOTE_MAX_TITLE      256
#define NOTE_MAX_TAGS       64
#define NOTE_MAX_LINKS      128
#define NOTE_MAX_BACKLINKS  256
#define NOTE_MAX_CONTENT    (10 * 1024 * 1024)  /* 10MB */
#define NOTE_TAG_MAX_LEN    64

typedef struct {
    char  path[NOTE_MAX_PATH];           /* File relative path */
    char  title[NOTE_MAX_TITLE];         /* Note title (first # heading) */
    char *content;                       /* Full content (must be freed manually) */
    size_t content_size;

    /* frontmatter */
    char  date[32];                      /* YAML date */
    char  tags_str[512];                 /* YAML tags field raw value */

    /* Parsed results */
    char  tags[NOTE_MAX_TAGS][NOTE_TAG_MAX_LEN];  /* All #tags and frontmatter tags */
    int   tag_count;

    char  links[NOTE_MAX_LINKS][NOTE_MAX_PATH];   /* Outgoing [[wikilinks]] */
    int   link_count;

    char  backlinks[NOTE_MAX_BACKLINKS][NOTE_MAX_PATH]; /* Incoming (computed by vault) */
    int   backlink_count;

    /* Timestamps */
    int64_t ctime;
    int64_t mtime;
} note_t;

/* ---- API ---- */

/// @brief Parse note content up to NOTE_MAX_CONTENT bytes, inclusive
utl_err_t note_parse(const char *path, const char *raw_content,
                     size_t raw_size, note_t *note);

/// @brief Free content, set it to NULL, and safely allow repeated calls on the same freed note
void note_free(note_t *note);

/// @brief Extract plain filename (without extension) from file path as default title
void note_filename_to_title(const char *path, char *title, size_t size);

/// @brief Extract the first NOTE_MAX_LINKS wikilinks in scan order
/// @note Excess links and NUL-terminated unclosed links are ignored with UTL_OK.
utl_err_t note_extract_links(const char *content, note_t *note);

/// @brief Extract the first NOTE_MAX_TAGS tags in scan order
/// @note Excess tags and body tags at least NOTE_TAG_MAX_LEN characters long are ignored.
utl_err_t note_extract_tags(const char *content, note_t *note);

UTL_EXTERN_C_END

#endif /* NOTE_H */
