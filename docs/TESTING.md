# Testing

## C11 test harness

Run the test suite from the repository root:

```sh
scons test
```

The test executable is written to:

```text
target/output/tests/vaultcode_tests
```

The test SConscript is loaded only for an explicit `test` target. Therefore,
ordinary `scons -c` does not clean the test executable or its object files.
Use the explicit test clean target:

```sh
scons -c test
```

The production static library is an explicit input of the test executable, so
`scons test` builds `target/output/static_lib/libvault.a` before linking the
test runner when the archive is absent or out of date.

## Note baseline coverage

The first note baseline contains five focused tests:

- filename-to-title conversion for a normal POSIX Markdown path;
- direct and aliased wikilink target extraction;
- extraction of one multi-character ASCII tag in body text;
- parsed-content copy ownership and `note_free` cleanup;
- a simple frontmatter `date` field.

All parser inputs are NUL-terminated strings and pass `strlen(content)` as
`raw_size`. The tests use no real files or directories and do not inspect note
timestamps.

The baseline intentionally does not define heading-level behavior, tag
deduplication, single-character tags, non-ASCII tags, general malformed input,
Markdown headings/code blocks as tag contexts, or timestamp failure semantics.

## Note error-path and capacity coverage

The second note batch covers required NULL arguments, content larger than
`NOTE_MAX_CONTENT`, and repeated safe calls to `note_free` on the same freed
note.

The third note batch confirms:

- `NOTE_MAX_CONTENT` is an inclusive content-size limit;
- excess links preserve the first `NOTE_MAX_LINKS` entries;
- excess body tags preserve the first `NOTE_MAX_TAGS` entries;
- body tags may contain at most `NOTE_TAG_MAX_LEN - 1` characters;
- a NUL-terminated unclosed wikilink is ignored.

The maximum-content tests share one writable static buffer. Generated link and
tag inputs use bounded `snprintf` calls whose results are checked. The suite
does not pass fictitious lengths, inaccessible buffers, or non-NUL-terminated
content to the parser.

## Vault baseline coverage

The first vault baseline contains four focused tests:

- opening an empty vault and reading zero note/tag counts;
- creating and retrieving one note through public APIs;
- listing two created notes without relying on result order;
- rejecting a duplicate exact relative path with `UTL_ERR_EXIST`.

Vault tests perform real file writes in unique directories created with
`mkdtemp`. They use `TMPDIR` when configured and otherwise the platform
temporary-directory definition. Each test closes caller-owned notes and vault
handles, removes its known root-level files, and removes its temporary
directory before evaluating assertions.

The baseline does not test path normalization, close/reopen scanning,
update/delete index consistency, backlinks, tag/search consistency, filesystem
failure injection, permissions, traversal, or concurrency.

## Vault delete regression coverage

`vault_delete_removes_note_safely` creates and deletes one note with separate
equal path strings. It confirms that the note returns `UTL_ERR_NOT_FOUND`
through the public get API immediately after deletion, then closes and reopens
the same temporary vault and confirms the deleted file is not rediscovered.
The test does not inspect hash internals or depend on allocator reuse.

## Vault backlink path-capacity coverage

Two focused tests exercise the fixed-size path used when backlink rebuilding
falls back from a direct wikilink target to the same target with `.md`:

- a 509-byte link cannot accept the suffix and must not use a truncated key
  that could match a different 511-byte note path;
- a 508-byte link can accept `.md` plus the terminating NUL and still resolves
  the target backlink.

The long paths use multiple directory components to stay below the filesystem
limit for an individual file name. Each test confirms that its target note is
retrievable through the public API before checking backlinks.
