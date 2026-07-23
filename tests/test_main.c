#include <stddef.h>
#include <stdio.h>

#include "test_harness.h"

bool test_harness_smoke(void);
bool note_filename_to_title_basic(void);
bool note_extract_links_basic(void);
bool note_extract_single_body_tag(void);
bool note_parse_copies_content(void);
bool note_parse_frontmatter_date(void);
bool note_parse_rejects_null_arguments(void);
bool note_parse_rejects_oversized_content(void);
bool note_extract_links_rejects_null_arguments(void);
bool note_extract_tags_rejects_null_arguments(void);
bool note_free_repeated_call_is_safe(void);
bool note_parse_accepts_max_content_size(void);
bool note_extract_links_stops_at_capacity(void);
bool note_extract_tags_stops_at_capacity(void);
bool note_extract_body_tag_length_boundary(void);
bool note_extract_links_ignores_unclosed_link(void);
bool vault_open_empty_and_info(void);
bool vault_create_and_get_note(void);
bool vault_list_notes_after_create(void);
bool vault_rejects_duplicate_note(void);
bool vault_delete_removes_note_safely(void);
bool vault_backlinks_skip_oversized_extension_key(void);
bool vault_backlinks_accept_max_extension_key(void);

int main(void)
{
    const test_case_t tests[] = {
        { "harness_smoke", test_harness_smoke },
        { "note_filename_to_title_basic", note_filename_to_title_basic },
        { "note_extract_links_basic", note_extract_links_basic },
        { "note_extract_single_body_tag", note_extract_single_body_tag },
        { "note_parse_copies_content", note_parse_copies_content },
        { "note_parse_frontmatter_date", note_parse_frontmatter_date },
        { "note_parse_rejects_null_arguments",
          note_parse_rejects_null_arguments },
        { "note_parse_rejects_oversized_content",
          note_parse_rejects_oversized_content },
        { "note_extract_links_rejects_null_arguments",
          note_extract_links_rejects_null_arguments },
        { "note_extract_tags_rejects_null_arguments",
          note_extract_tags_rejects_null_arguments },
        { "note_free_repeated_call_is_safe",
          note_free_repeated_call_is_safe },
        { "note_parse_accepts_max_content_size",
          note_parse_accepts_max_content_size },
        { "note_extract_links_stops_at_capacity",
          note_extract_links_stops_at_capacity },
        { "note_extract_tags_stops_at_capacity",
          note_extract_tags_stops_at_capacity },
        { "note_extract_body_tag_length_boundary",
          note_extract_body_tag_length_boundary },
        { "note_extract_links_ignores_unclosed_link",
          note_extract_links_ignores_unclosed_link },
        { "vault_open_empty_and_info", vault_open_empty_and_info },
        { "vault_create_and_get_note", vault_create_and_get_note },
        { "vault_list_notes_after_create",
          vault_list_notes_after_create },
        { "vault_rejects_duplicate_note",
          vault_rejects_duplicate_note },
        { "vault_delete_removes_note_safely",
          vault_delete_removes_note_safely },
        { "vault_backlinks_skip_oversized_extension_key",
          vault_backlinks_skip_oversized_extension_key },
        { "vault_backlinks_accept_max_extension_key",
          vault_backlinks_accept_max_extension_key },
    };
    const size_t test_count = sizeof(tests) / sizeof(tests[0]);
    size_t passed = 0;

    for (size_t i = 0; i < test_count; ++i) {
        if (tests[i].function()) {
            printf("[PASS] %s\n", tests[i].name);
            ++passed;
        } else {
            printf("[FAIL] %s\n", tests[i].name);
        }
    }

    printf("%zu/%zu tests passed\n", passed, test_count);
    return passed == test_count ? 0 : 1;
}
