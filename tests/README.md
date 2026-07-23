# VaultCode Tests

This directory contains the project's dependency-free C11 test harness.

The current suite has one harness smoke test, fifteen focused `note` tests,
four focused `vault` baseline tests, one vault delete regression test, and two
backlink path-capacity tests.
The first baseline tests are:

- `note_filename_to_title_basic`
- `note_extract_links_basic`
- `note_extract_single_body_tag`
- `note_parse_copies_content`
- `note_parse_frontmatter_date`

The error-path tests are:

- `note_parse_rejects_null_arguments`
- `note_parse_rejects_oversized_content`
- `note_extract_links_rejects_null_arguments`
- `note_extract_tags_rejects_null_arguments`
- `note_free_repeated_call_is_safe`

The capacity and safe malformed-input tests are:

- `note_parse_accepts_max_content_size`
- `note_extract_links_stops_at_capacity`
- `note_extract_tags_stops_at_capacity`
- `note_extract_body_tag_length_boundary`
- `note_extract_links_ignores_unclosed_link`

The first vault baseline tests are:

- `vault_open_empty_and_info`
- `vault_create_and_get_note`
- `vault_list_notes_after_create`
- `vault_rejects_duplicate_note`

The vault delete regression test is:

- `vault_delete_removes_note_safely`

The backlink path-capacity tests are:

- `vault_backlinks_skip_oversized_extension_key`
- `vault_backlinks_accept_max_extension_key`

The note tests use in-memory, NUL-terminated content and do not create files or
directories. Capacity inputs use bounded generated strings and a shared
writable static buffer for the content-size limit. They do not assert
timestamps or unresolved parsing semantics.

Each vault test creates a unique temporary directory with `mkdtemp`, closes
all vault and note resources, removes its known files and any test-created
empty parent directories, and then removes the temporary root before asserting
results. The list test treats result order as unspecified.

The delete regression uses independent equal path strings for create and
delete, verifies the note is absent immediately after deletion, and reopens the
same vault to confirm that the deleted file is not indexed again.

The backlink capacity tests use several directory components so that long
relative paths remain below the filesystem limit for any single component.
They verify that an over-capacity `.md` fallback key is not queried and that
the longest fallback key that fits remains valid.

## Run tests

From the repository root:

```sh
scons test
```

The command builds the production static library as an explicit dependency,
links `target/output/tests/vaultcode_tests`, and then runs that executable.
The command exits with a non-zero status if any test fails.

## Clean tests

Because `tests/SConscript` is loaded only when the `test` target is explicitly
requested, ordinary `scons -c` does not know about the test executable or test
object files. Use:

```sh
scons -c test
```

This removes the test target and the build dependencies reachable from the
`test` alias. A later `scons test` rebuilds them as needed.

## Add a test

Add a focused test function returning `bool`, declare it in `test_main.c`, and
add it to the `tests` table. Keep each business test focused on one behavior.
Known-defect characterization tests must be identified separately from tests
that describe confirmed behavior.
