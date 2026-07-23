# VaultCode Development Log

## 2026-07-22 — Minimal C11 test harness and SCons test target

### Goal and scope

- Add a dependency-free C11 test harness.
- Add `scons test` and a smoke test that validates only the harness.
- Document test execution and cleanup.
- Do not add `note` or `vault` business tests.
- Do not modify production source code.

### Key files read or modified

- Read: `AGENTS.md`, `SConstruct`, `src/SConscript`,
  `scripts/building/building.py`.
- Modified: `SConstruct`.
- Added: `tests/SConscript`, `tests/test_harness.h`, `tests/test_main.c`,
  `tests/test_smoke.c`, `tests/README.md`, `docs/TESTING.md`, and this log.

### Existing behavior understood

- `src/SConscript` declares `target/output/static_lib/libvault.a`.
- A SConscript loaded only for the `test` target is not visible to ordinary
  `scons -c`.
- A clean test invocation must model the static archive as a real node
  dependency rather than depend only on a `-lvault` linker flag.

### Options considered

- Linking with `LIBS=['vault']` would be concise, but would not make the
  archive dependency sufficiently explicit for this test target.
- Passing the `libvault.a` file node directly to `Program` gives SCons a
  concrete dependency and keeps production builder code unchanged.
- Loading the test SConscript unconditionally would make ordinary cleanup see
  test outputs, but could also add test targets to normal builds. Conditional
  loading keeps the existing default build behavior unchanged.

### Final choice and rationale

- Load `tests/SConscript` only when `test` is in `COMMAND_LINE_TARGETS`.
- Link the `libvault.a` file node directly into the test program.
- Define an always-run `test` alias whose action invokes the built executable.
- Document `scons -c test` as the required test cleanup command.

### Actual changes

- Added a one-test runner with a minimal `TEST_ASSERT` macro.
- Added `harness_smoke`; it asserts only that the harness executes.
- Added `target/output/tests/vaultcode_tests` as the test executable.
- Added test execution, extension, and cleanup documentation.
- No `note` or `vault` business tests were added.
- No file under `src/` was modified.

### Commands and results

- `scons test`: first infrastructure attempt exited 0 and built the runner,
  but did not print or execute the smoke test because the alias string action
  was ineffective. The action was replaced with an explicit subprocess call.
- `scons test`: exited 0; `[PASS] harness_smoke`; `1/1 tests passed`.
- `scons`: exited 0; existing production build was up to date; no test ran.
- `scons -c test`: exited 0; removed both test objects, the test executable,
  `libvault.a`, and the production objects reachable through that archive.
- `scons test` after cleanup: exited 0; rebuilt all archive objects, created
  `libvault.a`, linked the runner, and passed `1/1` tests.
- `scons -n -c`: exited 0; its dry-run removal list omitted the test objects
  and test executable, confirming that ordinary cleanup is insufficient.

### Warnings and failures

- The harness sources compiled without warnings.
- The clean production rebuild emitted six discarded-`const` warnings for
  `utl_hash_get` calls in `src/vault/vault.c` and one possible
  `snprintf` truncation warning in `prv_rebuild_backlinks`.
- These are existing production warnings and were not changed in this task.

### Discovered but unresolved issues

- Ordinary `scons -c` cannot clean targets declared only by the conditional
  test SConscript; use `scons -c test`.
- Existing production compiler warnings remain.

### Behavior still uncertain

- No business behavior was exercised, so this work does not confirm any
  `note` or `vault` API semantics.

### Next steps

- Add the previously reviewed business tests only after separate approval.
- Keep backlinks, update, and delete in separate, focused tests.
- Separate confirmed-behavior tests from known-defect characterization tests.

## 2026-07-22 — build.py and scons test configuration consistency review

### Goal and scope

- Read-only review of `build.py`, `SConstruct`, the production and test
  SConscript files, and `scripts/`.
- Confirm command dispatch, `.env` lifecycle, SCons Environment inheritance,
  and the risk of mixing artifacts built with different configurations.
- Do not modify build or production files and do not run a build.

### Key files read

- `build.py`
- `SConstruct`
- `src/SConscript`
- `tests/SConscript`
- `scripts/helper/env_helper.py`
- `scripts/helper/path_helper.py`
- `scripts/arch/*.py`
- `scripts/building/building.py`
- `scripts/menuconfig/mconfig.py`

### Existing behavior understood

- The exact command `python3 build.py -- build` does not set `args.build` and
  therefore does not invoke SCons. The effective option is `--build`.
- `python3 build.py --build` eventually invokes plain `scons` without
  forwarding unknown arguments.
- `--arch` recreates `.env`; architecture scripts write toolchain, flags,
  paths, and architecture values both to `.env` and the current process.
- `--build` without `--arch` only loads the existing `.env`.
- `SConstruct` loads the same root `.env`, so direct `scons test` reads the
  same persisted configuration when run from the repository root.

### Options and tradeoffs considered

- Reusing a shared or cloned SCons Environment would provide stronger
  production/test parity, but the current test SConscript creates an
  independent Environment.
- Keeping common output paths is simple, but makes architecture switches rely
  on SCons signatures instead of isolating artifacts by architecture.

### Final assessment and rationale

- There is no launcher-specific temporary configuration that inherently makes
  a subsequent direct `scons test` use different settings when `.env` remains
  unchanged.
- Production and test builds do not use the same Environment and there is no
  `Clone()` relationship. The test Environment copies only the current `CC`
  and `CFLAGS`, plus test include and link libraries.
- `libvault.a` is a concrete test dependency and is built through the
  production graph using the current `.env`, which mitigates stale-library
  reuse in normal SCons operation.
- Configuration parity is still incomplete: test objects do not inherit all
  production tools, system include paths, or module-added flags.
- All architectures share the same target paths, so concurrent builds,
  externally copied artifacts, or abnormal signature state can still create a
  mixed-artifact risk.

### Commands and results

- Used read-only file inspection and text search for `Environment`, `Clone`,
  compiler flags, environment loading, environment writing, and subprocess
  calls.
- No `build.py` or SCons command was executed.
- No file was modified during the review.

### Warnings, failures, and unresolved issues

- No `LINKFLAGS` configuration was found.
- The test Environment does not fully inherit production configuration.
- Architecture-specific builds are not separated into distinct output trees.
- Direct `scons test` requires valid root `.env` and `.config` files.

### Behavior still uncertain

- The operational outcome of concurrent builds using different architecture
  configurations was not tested.

### Next steps

- If full production/test configuration parity becomes a requirement, define
  a shared base Environment or clone an authoritative environment.
- Consider architecture-specific output directories separately; no design
  change was made in this review.

## 2026-07-22 — Test infrastructure read-only self-review

### Goal and scope

- Read-only self-review of the minimal test infrastructure.
- Check target recognition, repeated execution, dependency modeling, failure
  propagation, assertion safety, compile configuration, and Git scope.
- Do not run SCons or modify files during the review.

### Key files read

- `SConstruct`
- `tests/SConscript`
- `tests/test_harness.h`
- `tests/test_main.c`
- `tests/test_smoke.c`
- `tests/README.md`
- `docs/TESTING.md`
- `docs/DEVELOPMENT_LOG.md`
- `src/SConscript`
- `scripts/building/building.py`

### Existing behavior understood

- Membership testing against `COMMAND_LINE_TARGETS` recognizes the explicit
  `test` target for `scons test`, `scons -c test`, and `scons -n test`.
- `-n` recognizes and displays the planned test action but intentionally does
  not execute it.
- `AlwaysBuild(test_alias)` makes the runner action execute on every real
  `scons test`, even when the test executable is up to date.
- The archive is created with `File(...)` and passed as a `Program` source, so
  it is a concrete SCons node dependency rather than only a linker name.

### Options and tradeoffs considered

- A direct failure injection would empirically confirm non-zero propagation,
  but it would require changing the only smoke test and was outside this
  read-only review.
- Static code review can verify the complete failure path without modifying the
  suite: assertion failure returns `false`, `main` returns `1`,
  `subprocess.call` returns that code, and SCons treats it as action failure.

### Final assessment and rationale

- Target recognition, repeat execution, concrete archive dependency, and
  failure-code propagation are correctly modeled.
- `TEST_ASSERT` evaluates its expression once, uses `do { ... } while (0)` for
  safe `if`/`else` use, and reports the expression, source file, and line.
- Production sources use the existing Builder configuration. The minimal C11
  smoke harness uses the existing `CC` and base `CFLAGS`, which are sufficient
  for the current test, but it does not fully inherit the production
  Environment or module-added flags.
- No production source, application source, `build.py`, or file under
  `scripts/` had a tracked diff. The changes were limited to `SConstruct`,
  `tests/`, and the required testing/development documentation.

### Commands and results

- `git status --short` reported modified `SConstruct` and untracked `docs/`
  and `tests/`.
- `git diff -- SConstruct tests docs/TESTING.md docs/DEVELOPMENT_LOG.md`
  displayed only `SConstruct` because Git does not include untracked files in
  an ordinary diff.
- `git diff -- src app build.py scripts` produced no output.
- Used read-only file inspection and text searches; SCons was not executed
  because its configuration phase performs header-copy side effects.

### Warnings, failures, and unresolved issues

- Failure propagation is established by code review but has not yet been
  exercised with an intentionally failing test.
- Full production/test Environment parity remains unresolved.

### Behavior still uncertain

- No failure test was injected, so the exact displayed SCons failure text was
  not observed.

### Next steps

- Keep future business tests focused on one confirmed behavior.
- Keep known-defect characterization tests separate.
- Revisit shared Environment inheritance before business headers require
  production-specific macros or include configuration.

## 2026-07-23 — Note baseline tests and tag boundary fix

### Goal and scope

- Implement the approved first batch of five focused note baseline tests.
- Extend the dependency-free harness only with the typed assertions needed by
  those tests.
- Keep the existing smoke test and production `libvault.a` dependency.
- Do not add file fixtures or define unresolved note parsing behavior.
- If a test exposed a production defect, make only the separately approved
  minimal fix in `src/note/note.c`.

### Key files read or modified

- Read: `src/note/note.h`, `src/note/note.c`, `src/note/SConscript`,
  `src/utils/common/utl_err.h`, and the existing test infrastructure.
- Added: `tests/test_note.c`.
- Modified: `tests/test_main.c`, `tests/test_harness.h`,
  `tests/SConscript`, `tests/README.md`, `docs/TESTING.md`, and
  `src/note/note.c`.

### Existing behavior understood

- `note_parse` consumes caller-provided NUL-terminated content, allocates the
  `note.content` copy, and only uses `path` for an optional `stat` call.
- `note_free` releases only the allocated content.
- Direct tag extraction also reads `note.tags_str`, so every test initializes
  `note_t` with `{0}`.
- The original test Environment needed `src/note` and `src/utils/common`
  include paths to compile against the public note header.

### Options and tradeoffs considered

- Real Markdown files were unnecessary because none of the approved assertions
  concern timestamps or file I/O.
- A single generic assertion system would add complexity, so explicit int,
  size, string, null, and non-null macros were added without `_Generic`.
- Weakening the body-tag test or moving the tag to offset zero would hide the
  observed defect and was rejected.

### Final choice and rationale

- Add exactly the five approved tests in their approved order after the smoke
  test.
- Link only through the existing production archive; do not compile `note.c`
  separately in the test target.
- Preserve the input `"body text #alpha end"` and repair the production pointer
  basis after the baseline exposed the defect.

### Actual changes

- Added baseline tests for filename title extraction, wikilinks, one body tag,
  content-copy ownership, and frontmatter date parsing.
- Added single-evaluation assertion macros for int, `size_t`, strings, null,
  and non-null values. The macros use `do { ... } while (0)` and print expected
  and actual values with source location.
- Added note/common include paths and `test_note.c` to the test build.
- Changed one call in `note_extract_tags` from
  `prv_is_word_boundary(p, p - content)` to
  `prv_is_word_boundary(content, p - content)`.

### Defect details

- `prv_is_word_boundary(s, idx)` checks `s[idx - 1]`, so `s` must be the base
  pointer used to calculate `idx`.
- For `"body text #alpha end"`, `#` is at offset 10. Before the fix, the call
  passed `p` and index 10, so the helper inspected `p[9]`, the final character
  `'d'`, rather than the character before `#`.
- After the fix, it inspects `content[9]`, the intended space before `#`.
- Repository search found no other call to `prv_is_word_boundary`, so no second
  occurrence required correction.

### Commands and results

- Initial `scons test`: exited 2; five of six tests passed.
  `note_extract_single_body_tag` expected `tag_count == 1` but observed `0`.
- `scons test` after the minimal fix: exited 0; all six tests passed.
- Repeated `scons test`: exited 0; all six tests ran and passed again while the
  executable was already up to date.
- `scons`: exited 0; relinked `app_example`, `vault-cli`, and `vaultd` against
  the updated archive.
- `git diff --check`: exited 0 with no whitespace errors.
- `git diff -- src/note/note.c`: showed only the one-line pointer-basis fix.
- `git diff -- tests`: produced no output because `tests/` is currently
  untracked; the directory was inspected directly.
- `git status --short`: showed modified `SConstruct` and `src/note/note.c`,
  plus untracked `docs/` and `tests/`.

### Warnings and failures

- The initial tag test failure was caused by the production pointer/index basis
  mismatch described above.
- The successful rebuild emitted no compiler warnings.

### Discovered but unresolved issues

- The test Environment still copies only the required subset of the production
  configuration rather than cloning a shared Environment.
- The parsing ambiguities excluded from this baseline remain unresolved.

### Behavior still uncertain

- Heading-level semantics, tag deduplication, single-character tags,
  non-ASCII tags, malformed input, capacity limits, Markdown context handling,
  and timestamp failure behavior remain unspecified by this test batch.

### Next steps

- Keep subsequent note tests focused and obtain confirmation before asserting
  any unresolved parsing behavior.
- Add vault business tests only in a separately approved batch.

## 2026-07-23 — Second note error-path test batch

### Goal and scope

- Add the approved five note error-path tests while preserving the existing
  six-test baseline.
- Cover required NULL arguments, oversized input, and repeated `note_free`.
- Document the confirmed repeated-free contract without changing the API or
  production implementation.
- Do not add `note_free(NULL)` coverage or unrelated parser behavior.

### Key files read or modified

- Read: `src/note/note.c`, `src/utils/common/utl_err.h`, and the existing test
  infrastructure.
- Modified: `src/note/note.h`, `tests/test_note.c`, `tests/test_main.c`, and
  this log.
- Not modified: `src/note/note.c`, `tests/test_harness.h`, and
  `tests/SConscript`.

### Existing behavior understood

- `note_parse` returns `UTL_ERR_NULL` for each required NULL pointer and
  `UTL_ERR_FULL` when `raw_size` exceeds `NOTE_MAX_CONTENT`.
- `note_extract_links` and `note_extract_tags` return `UTL_ERR_NULL` for either
  required NULL pointer.
- `note_free` frees `content` and sets it to NULL, so a repeated call on the
  same freed note is safe.
- The existing harness already provides all required integer, NULL, and
  non-NULL assertions.

### Options and tradeoffs considered

- NULL cases for the same API were grouped into one test, with an immediate
  assertion and a separately named result for each call.
- A short input paired with a fictitious oversized length would rely on the
  current check ordering, so the oversized test instead uses a real static
  `NOTE_MAX_CONTENT + 2` byte buffer.
- Dynamic allocation was unnecessary for the oversized input. Static writable
  storage avoids a large stack object and avoids placing a const array in
  read-only data.
- `note_free(NULL)` remains excluded because NULL-safe behavior was not added
  to the public contract.

### Final choice and rationale

- Add exactly the five approved tests and register them after the existing
  baseline tests.
- Assert only the specified error codes; do not assert output state after an
  error.
- Allocate one byte for the repeated-free test, verify allocation, then verify
  that `content` is NULL after both calls.
- Update only the `note_free` API comment to state that content is cleared and
  repeated calls on the same freed note are safe.

### Actual changes

- Added `note_parse_rejects_null_arguments`.
- Added `note_parse_rejects_oversized_content` with a real writable static
  buffer.
- Added `note_extract_links_rejects_null_arguments`.
- Added `note_extract_tags_rejects_null_arguments`.
- Added `note_free_repeated_call_is_safe`.
- Registered all five tests in `tests/test_main.c`, increasing the suite from
  6 to 11 tests.
- Clarified the confirmed repeated-free behavior in `src/note/note.h` without
  changing the function signature or ABI.

### Commands and results

- First `scons test`: exited 0; all 11 tests passed. The modified public header
  caused dependent production objects to rebuild.
- Second `scons test`: exited 0; no recompilation was needed, but the test
  action still ran and all 11 tests passed.
- `scons`: exited 0; rebuilt application objects affected by the copied public
  header.
- `git diff --check`: exited 0 with no whitespace errors.
- `git diff -- src/note/note.c`: showed only the previously approved one-line
  tag-boundary fix; this batch did not modify the implementation.
- `git diff -- src/note/note.h`: showed only the `note_free` contract comment.
- `git diff -- tests`: produced no output because `tests/` remains untracked;
  the files were inspected directly.
- `git status --short`: showed the existing `SConstruct` and `note.c` changes,
  the new `note.h` comment, and the untracked `docs/` and `tests/` directories.

### Warnings and failures

- No test failed and the new test sources compiled without warnings.
- The header-triggered rebuild emitted six existing discarded-`const` warnings
  and one existing possible `snprintf` truncation warning in
  `src/vault/vault.c`.
- The production build emitted one existing possible `snprintf` truncation
  warning in `app/server.c`.
- These warnings are outside this task and were not modified.

### Discovered but unresolved issues

- Existing production compiler warnings remain.
- `note_free(NULL)` is currently accepted by the implementation but remains
  outside the documented public contract and this test batch.

### Behavior still uncertain

- No output-state guarantee is defined for note operations that return an
  error; the new tests intentionally do not establish one.
- Previously excluded parsing semantics remain unspecified.

### Next steps

- Keep future error paths isolated and confirm their public semantics before
  adding tests.
- Review the complete uncommitted scope before staging or committing it.

## 2026-07-23 — Third note capacity and safe malformed-input test batch

### Goal and scope

- Add the five approved note capacity-boundary and safe malformed-input tests.
- Preserve all 11 existing tests and increase the suite to 16 tests.
- Document the confirmed inclusive content limit, link/tag saturation, body
  tag length, and unclosed wikilink behavior.
- Do not test path, title, backlinks, unclosed frontmatter, embedded NUL,
  fictitious lengths, or non-NUL-terminated input.
- Do not modify parser or vault production implementations.

### Key files read or modified

- Read: `src/note/note.c`, the existing note tests, harness, SCons test script,
  and testing documentation.
- Modified: `src/note/note.h`, `tests/test_note.c`, `tests/test_main.c`,
  `tests/README.md`, `docs/TESTING.md`, and this log.
- Not modified: `src/note/note.c`, `src/vault/vault.c`,
  `tests/test_harness.h`, and `tests/SConscript`.

### Existing behavior understood

- `NOTE_MAX_CONTENT` is accepted as an inclusive content-size limit when
  allocation succeeds; larger input is rejected by the existing error test.
- Link extraction keeps the first `NOTE_MAX_LINKS` valid links and ignores
  later links while returning `UTL_OK`.
- Tag extraction keeps the first `NOTE_MAX_TAGS` body tags and ignores later
  tags while returning `UTL_OK`.
- A body tag can contain at most `NOTE_TAG_MAX_LEN - 1` characters; a tag at
  or above the array width is ignored rather than truncated.
- A NUL-terminated unclosed wikilink is ignored with `UTL_OK`.

### Options and tradeoffs considered

- Reusing one file-level writable static buffer avoids a second 10 MiB object,
  a large stack allocation, and a large const object in read-only data.
- Each capacity-buffer test initializes its own required range, so tests do
  not depend on execution order.
- Link and tag inputs are generated with checked `snprintf` calls rather than
  unchecked concatenation.
- Boundary values are captured before freeing maximum-size parsed content, so
  cleanup occurs before assertions can return from the test.

### Final choice and rationale

- Move the existing oversized-input buffer to one shared file-level writable
  array and reuse it for the inclusive maximum test.
- Generate 129 unique links and 65 unique multi-character ASCII body tags in
  bounded local buffers.
- Use separate zero-initialized notes and fixed arrays for 63- and
  64-character body tags.
- Use an ordinary NUL-terminated string for the unclosed wikilink test.
- Update only public comments and testing documentation; make no ABI or parser
  implementation change.

### Actual changes

- Added `note_parse_accepts_max_content_size`.
- Added `note_extract_links_stops_at_capacity`.
- Added `note_extract_tags_stops_at_capacity`.
- Added `note_extract_body_tag_length_boundary`.
- Added `note_extract_links_ignores_unclosed_link`.
- Registered all five tests, increasing the suite from 11 to 16.
- Updated the note API comments and test documentation for the confirmed
  behavior.

### Commands and results

- First `scons test`: exited 0; all 16 tests passed after rebuilding affected
  test and production objects.
- Second `scons test`: exited 0; no recompilation was needed, the test action
  still executed, and all 16 tests passed.
- `scons`: exited 0; rebuilt application objects affected by the copied public
  header.
- `git diff --check`: exited 0 with no whitespace errors.
- `git diff -- src/note/note.c`: showed only the previously approved one-line
  tag-boundary fix; this batch did not modify the implementation.
- `git diff -- src/note/note.h`: showed documentation-only contract changes.
- `git diff -- tests`: produced no output because `tests/` remains untracked;
  the files were inspected directly.
- `git status --short`: showed the existing `SConstruct` and `note.c` changes,
  the accumulated `note.h` comments, and untracked `docs/` and `tests/`.

### Warnings and failures

- No test failed and the new test sources compiled without warnings.
- The header-triggered test rebuild emitted six existing discarded-`const`
  warnings and one existing possible `snprintf` truncation warning in
  `src/vault/vault.c`.
- The production build emitted one existing possible `snprintf` truncation
  warning in `app/server.c`.
- These warnings remain outside this task and were not modified.

### Discovered but unresolved issues

- Existing production compiler warnings remain.
- Unsafe length/NUL mismatches and other excluded malformed input remain
  outside the test contract.

### Behavior still uncertain

- Path, title, backlink, frontmatter-malformation, embedded-NUL, and invalid
  buffer semantics remain unconfirmed.

### Next steps

- Keep any further malformed-input work separate from unsafe buffer-contract
  testing.
- Review the complete uncommitted scope before staging or committing.

### Verification addendum

- After the final wording refinement in `src/note/note.h`, another
  `scons test` exited 0 and passed all 16 tests. The header-triggered rebuild
  repeated the same existing `src/vault/vault.c` warnings.
- A final `scons` exited 0 and repeated the existing `app/server.c`
  `snprintf` warning.
- Final `git diff --check` exited 0; the requested note source/header/test
  diffs and worktree status were inspected again.

## 2026-07-23 — First vault baseline test batch

### Goal and scope

- Add four approved `vault.h` API baseline tests after confirming the existing
  16-test suite was stable.
- Cover empty open/info, create/get, unordered list, and exact-path duplicate
  rejection.
- Use unique temporary directories with reliable ordinary-failure cleanup.
- Do not test reopen scanning, update/delete index consistency, backlinks,
  tag/search consistency, path normalization, failure injection, traversal,
  permissions, or concurrency.

### Key files read or modified

- Read: `src/vault/vault.c`, `src/search/utl_search.h`,
  `src/search/utl_search.c`, Linux file/mutex implementations, hash behavior,
  public API call sites, and the existing test infrastructure.
- Added: `tests/test_vault.c`.
- Modified: `src/vault/vault.h`, `tests/test_main.c`,
  `tests/SConscript`, `tests/README.md`, `docs/TESTING.md`, and this log.
- Not modified: `src/vault/vault.c`, note/search/platform implementations, or
  `tests/test_harness.h`.

### Existing behavior understood

- `vault_open` returns a caller-owned opaque handle released by `vault_close`.
- `vault_close` releases in-memory resources without deleting the vault
  directory or note files.
- `vault_note_create` performs a real file write, parses the note, and adds it
  to in-memory indexes.
- `vault_note_get` copies the note and duplicates content for the caller, who
  releases it with `note_free`.
- List order follows hash iteration and has no ordering guarantee.
- Duplicate detection compares exact relative-path strings; no path
  normalization occurs.
- `src/vault/SConscript` builds only `vault.c`; the separate legacy
  `utl_vault.c` implementation is outside this test scope.

### Options and tradeoffs considered

- `mkdtemp` with `TMPDIR` or the platform temporary-directory definition
  avoids fixed paths, working-directory dependence, and test collisions.
- Root-level known file names allow explicit cleanup without a recursive
  deletion helper.
- Delaying assertions until after `note_free`, `vault_close`, file removal,
  and `rmdir` ensures ordinary test failures do not bypass cleanup.
- Close/reopen was deferred because it combines persistence, directory
  scanning, parsing, search indexing, and backlink rebuilding.

### Final choice and rationale

- Add four independent tests, each with its own temporary root.
- Assert only public results: return codes, empty counts, retrieved path and
  content, unordered list membership, and exact-path duplicate rejection.
- Keep the fixture private to `test_vault.c`; the existing harness remains
  sufficient.
- Document handle/content ownership, persistent files after close, exact-path
  duplicate behavior, and unspecified list order in `vault.h`.

### Actual changes

- Added `vault_open_empty_and_info`.
- Added `vault_create_and_get_note`.
- Added `vault_list_notes_after_create`.
- Added `vault_rejects_duplicate_note`.
- Added checked temporary-root creation and known-file cleanup helpers.
- Added `test_vault.c` plus vault/search include paths to the test build.
- Registered the four tests, increasing the suite from 16 to 20.
- Updated public contract comments and stable testing documentation.

### Commands and results

- Before implementation, first `scons test`: exited 0; all 16 existing tests
  passed.
- Before implementation, second `scons test`: exited 0; all 16 tests executed
  and passed again.
- Before implementation, `scons`: exited 0; production build was up to date.
- Before implementation, `git diff --check`: exited 0.
- First post-change `scons test`: exited 0; all 20 tests passed.
- Second post-change `scons test`: exited 0; no recompilation was needed, the
  test action still ran, and all 20 tests passed.
- Post-change `scons`: exited 0; rebuilt and linked affected applications.
- `git diff --check`: exited 0 with no whitespace errors.
- `git diff -- src/vault/vault.c`: produced no output.
- `git diff -- src/vault/vault.h`: showed documentation-only contract changes.
- `git diff -- tests`: produced no output because `tests/` remains untracked;
  the files were inspected directly.

### Warnings and failures

- No test failed and `tests/test_vault.c` compiled without warnings.
- The header-triggered rebuild repeated six existing discarded-`const`
  warnings and one existing possible `snprintf` truncation warning in
  `src/vault/vault.c`.
- The production build repeated one existing possible `snprintf` truncation
  warning in `app/server.c`.
- These warnings remain outside this task and were not modified.

### Discovered but unresolved issues

- `vault_open` ignores several initialization and scan return values.
- Create ignores write and index return values; get does not propagate
  content-duplication failure.
- Folder APIs do not use the vault mutex despite the broad thread-safe claim.
- The legacy `utl_vault.h` API is present, but its implementation is not in
  the current production source whitelist.

### Behavior still uncertain

- Path normalization is unspecified and absent from the current implementation.
- Close/reopen behavior can depend on directory-entry type reporting.
- Failure-path cleanup and partial initialization behavior are not covered by
  this baseline.

### Next steps

- Keep close/reopen scanning in a separate batch.
- Design update, delete, search, tag, and backlink consistency tests
  independently rather than combining them.

## 2026-07-23 — Backlink extension-key truncation fix

### Goal and scope

- Prevent backlink rebuilding from querying the note index with a truncated
  `<wikilink>.md` fallback key.
- Add one regression test for the old collision risk and one test for the
  longest fallback key that fits.
- Keep `NOTE_MAX_PATH` unchanged, add no public error code or broader vault
  path contract, and leave the six existing discarded-`const` warnings for a
  separate batch.

### Key files read or modified

- Read: `AGENTS.md`, `src/vault/vault.c`, `src/vault/vault.h`,
  `tests/test_harness.h`, `tests/SConscript`, and the existing vault tests and
  testing documentation.
- Modified: `src/vault/vault.c`, `tests/test_vault.c`,
  `tests/test_main.c`, `tests/README.md`, `docs/TESTING.md`, and this log.
- Not modified in this batch: `src/vault/vault.h`, note/hash implementations,
  or the test harness.

### Existing behavior understood

- Backlink rebuilding first queries the exact wikilink target. It appends
  `.md` only when that direct query fails.
- The fallback array has `NOTE_MAX_PATH` bytes. A 508-byte link plus `.md` and
  the terminating NUL fits exactly; a 509-byte link does not.
- The old `snprintf` path for a 509-byte link produced a truncated
  `link + ".m"` key. If a different 511-byte note path had that value, the
  backlink could be attached to the wrong note.
- A proposed collision where the link exactly equaled an existing `.md` path
  could not reach the fallback branch because the direct query would succeed.
  The approved regression therefore uses the actually reachable `.m`
  collision path.

### Options and tradeoffs considered

- Enlarging the temporary array would remove this warning but would not enforce
  the confirmed rule that an incomplete identity key must never be queried.
- Checking `snprintf` would be safe, but explicit length checking plus bounded
  copies makes the capacity boundary direct and avoids format-truncation
  diagnostics.
- Defining a new public path-length error was rejected as unnecessary scope
  expansion.

### Final choice and rationale

- Measure the stored link with `strlen`.
- Skip the fallback when the link is longer than
  `sizeof(with_md) - sizeof(".md")`.
- Otherwise copy the link bytes and then copy `".md"` including its NUL.
- Keep the direct lookup behavior unchanged.

This preserves the existing path capacity and guarantees that
`utl_hash_get` never receives a truncated fallback key.

### Actual changes

- Replaced the fallback `snprintf` in `prv_rebuild_backlinks` with an explicit
  length guard and two `memcpy` calls.
- Added `vault_backlinks_skip_oversized_extension_key`, using a 509-byte link
  and a separately created 511-byte `link + ".m"` collision note.
- Added `vault_backlinks_accept_max_extension_key`, using a 508-byte link and
  the valid 511-byte `link + ".md"` target.
- Both tests use three 120-byte directory components, keep individual
  filesystem components below `NAME_MAX`, verify the target through
  `vault_note_get`, and explicitly clean files and empty parent directories.
- Registered the two tests and documented the stable coverage, increasing the
  suite from 20 to 22 tests.

### Commands and results

- First `scons test`: exited 0; rebuilt the changed production and test
  objects; all `22/22` tests passed.
- The first build emitted the six existing discarded-`const` warnings in
  `vault.c`; the previous `snprintf` format-truncation warning was absent.
- The changed test sources compiled without warnings.
- Second `scons test`: exited 0; no recompilation was needed, the test action
  still executed, and all `22/22` tests passed.
- `scons`: exited 0; linked `app_example`, `vault-cli`, and `vaultd`.
- `git diff --check`: exited 0 with no whitespace errors.
- `git diff -- src/vault/vault.c`: showed only the approved length guard and
  two bounded copies.
- `git diff -- src/vault/vault.h`: showed only pre-existing documentation
  changes from the earlier vault baseline batch.
- `git diff -- tests`: produced no output because `tests/` remains untracked;
  the modified test files were inspected directly.
- `git status --short`: retained the existing dirty worktree and added the
  intended tracked modification to `src/vault/vault.c`.

### Warnings and failures

- No test or build command failed.
- The format-truncation warning at the backlink fallback was eliminated.
- Six existing `utl_hash_get` discarded-`const` warnings remain intentionally
  deferred.
- The ordinary production build reused existing application objects, so it
  did not re-emit or re-evaluate unrelated application-source warnings.

### Discovered but unresolved issues

- The hash lookup API still declares its read-only lookup key as `void *`,
  causing the six deferred warnings.
- Broader vault path-length validation and public error semantics remain
  unspecified and were not changed.

### Behavior still uncertain

- This batch does not define behavior for arbitrary overlong public vault
  paths; it only prevents a non-fitting backlink fallback key from being used.
- Close/reopen behavior for non-`.md` paths remains outside this test because
  the collision note is verified through the current public create/get APIs.

### Next steps

- Address the six hash lookup const-correctness warnings as a separate change.
- Keep broader path contracts and reopen scanning behavior in separately
  designed test batches.

## 2026-07-23 — `utl_hash_get` const-correctness fix

### Goal and scope

- Correct the read-only query-key contract of `utl_hash_get`.
- Eliminate the six discarded-`const` warnings emitted by `vault.c`.
- Limit changes to `utl_hash_get` and internal hashing/comparison helpers whose
  key parameters are read-only on every call path.
- Do not modify `utl_hash_remove`, `utl_hash_contains`, other hash APIs,
  vault public APIs, backlink path construction, note code, or tests.

### Key files read or modified

- Read: `src/utils/data_struct/hash/utl_hash.h`,
  `src/utils/data_struct/hash/utl_hash.c`, every `utl_hash_get` call site,
  `src/search/utl_search.c`, `src/vault/vault.c`, and the current test build
  configuration.
- Modified: `src/utils/data_struct/hash/utl_hash.h`,
  `src/utils/data_struct/hash/utl_hash.c`, and this log.
- Not modified in this batch: `src/vault/vault.c`, `src/vault/vault.h`,
  `utl_hash_remove`, `utl_hash_contains`, tests, or testing instructions.

### Existing behavior understood

- `utl_hash_get` checks the query key, hashes it, compares it against stored
  keys, and returns the stored value. It does not modify, save, free, or take
  ownership of the query key.
- `prv_hash` and `prv_key_eq` are also used by put/remove paths, but both
  helpers only inspect their key arguments on every path. `utl_hash_put`
  stores its key separately after those helpers return.
- Stored node keys remain `void *`; this change does not alter stored-key
  type, ownership, or lifetime.
- No hash/comparison callback typedefs or function-pointer assignments exist
  in this implementation.
- The repository has no dedicated hash unit-test source. Existing vault and
  search behavior exercises successful and unsuccessful hash queries.

### Options and tradeoffs considered

- Copying query keys or casting away `const` would hide the incorrect contract
  without representing actual ownership and was rejected.
- Changing remove/contains or all hash APIs would exceed the approved batch.
- A new runtime test would add little coverage for a source-level qualifier
  correction; rebuilding all consumers with warnings enabled and running the
  existing suite directly verifies the affected paths.

### Final choice and rationale

- Change only the public declaration and definition of the `utl_hash_get`
  query key from `void *` to `const void *`.
- Propagate `const void *` through `prv_hash` and both parameters of
  `prv_key_eq`, since those helpers never mutate, store, free, or transfer
  their arguments.
- Leave all callers unchanged so their existing pointer types convert
  naturally to the corrected read-only interface.

This is a source-level const-correctness contract correction. The C symbol,
argument count, pointer representation, calling convention, return values, and
runtime lookup behavior remain unchanged, so the binary ABI calling convention
is unchanged; consumers must still recompile against the updated declaration.

### Actual changes

- Updated `utl_hash_get` in `utl_hash.h` and `utl_hash.c` to accept
  `const void *key`.
- Updated private `prv_hash` and `prv_key_eq` key parameters to
  `const void *`.
- Added no casts, key copies, warning suppressions, runtime tests, or changes
  to vault/search call sites.

### Commands and results

- Initial `git status --short`: confirmed and preserved the existing dirty
  worktree.
- Pre-change `scons test`: exited 0; all `22/22` tests passed. Recompiling
  `vault.c` emitted exactly the six known discarded-`const` warnings and no
  format-truncation warning.
- Pre-change `scons`: exited 0; it linked the three applications but reused
  current objects, so it did not independently reprint source diagnostics.
- Pre-change standalone production-options compile of `vault.c`: exited 0 and
  reproduced the same six complete discarded-`const` diagnostics at the
  create, get, update, delete, backlinks, and notes-by-tag query calls.
- No targeted hash test was run because the project has no dedicated hash test
  target or hash test source.
- First post-change `scons test`: exited 0; rebuilt hash, search, and vault
  objects without warnings; all `22/22` tests passed.
- Second post-change `scons test`: exited 0; no recompilation was needed, the
  test action still executed, and all `22/22` tests passed.
- Post-change `scons`: exited 0 and linked `app_example`, `vault-cli`, and
  `vaultd`.
- Post-change standalone production-options compile of `vault.c`: exited 0
  with empty diagnostic output.
- `git diff --check`: exited 0.
- The hash diff contains only the approved declaration, definition, and two
  private-helper qualifier changes.
- `git diff -- src/vault/vault.c` contains only the earlier backlink
  truncation fix; this batch made no vault source change.
- `git diff -- tests` remains empty because `tests/` is untracked; no test
  file was changed in this batch.

### Warnings and failures

- No test, compile, build, or check failed.
- All six discarded-`const` warnings are gone.
- The earlier `snprintf` format-truncation warning remains absent.
- No incompatible-pointer-types, discarded-qualifiers, or other new warning
  appeared while rebuilding affected consumers.

### Discovered but unresolved issues

- `utl_hash_remove` and `utl_hash_contains` retain their existing signatures as
  explicitly required.
- The explicit cast used by the existing vault delete/remove path remains
  outside this batch.

### Behavior still uncertain

- No additional stored-key ownership contract was defined; `utl_hash_put`
  continues to store the caller-provided key pointer under its existing rules.
- Source compatibility for external code that assigns `utl_hash_get` to an
  exactly typed function pointer cannot be guaranteed without recompilation,
  although no such assignment exists in this repository.

### Next steps

- Keep any review of remove/contains const-correctness in a separate approved
  batch.
- Do not combine that future work with stored-key ownership or broader hash API
  redesign.

## 2026-07-23 — `utl_hash_remove` const-correctness fix

### Goal and scope

- Correct the read-only lookup-key contract of `utl_hash_remove`.
- Remove the explicit discarded-`const` cast in `vault_note_delete`.
- Limit changes to the remove declaration/definition and that single vault
  call; do not modify `utl_hash_contains`, insertion/get behavior, backlink
  logic, vault public APIs, or tests.

### Key files read or modified

- Read: `src/utils/data_struct/hash/utl_hash.h`,
  `src/utils/data_struct/hash/utl_hash.c`, every `utl_hash_remove` call site,
  shared hash/compare helpers, `src/vault/vault.c`, and the current test build.
- Modified: `src/utils/data_struct/hash/utl_hash.h`,
  `src/utils/data_struct/hash/utl_hash.c`, `src/vault/vault.c`, and this log.
- Not modified in this batch: `utl_hash_contains`, `utl_hash_get`,
  `utl_hash_put`, vault public headers, note/search behavior, tests, or testing
  instructions.

### Existing behavior understood

- `utl_hash_remove` uses its caller-provided key only to calculate a hash and
  compare against keys already stored in hash nodes.
- It does not modify, save, free, or transfer ownership of the caller's query
  key.
- On success, remove unlinks and frees only the matching `hash_node_t`; it does
  not free the node's key or value. On failure, it frees nothing.
- The hash implementation has no key/value destructor callback or callback
  typedef.
- The only repository call is `vault_note_delete`, which receives a borrowed
  `const char *rel_path` and previously cast it to `void *`.
- Shared `prv_hash` and `prv_key_eq` helpers were already made read-only in the
  preceding `utl_hash_get` batch and require no further change.

### Options and tradeoffs considered

- Replacing the old cast with another cast or copying `rel_path` would only
  conceal the query contract and was rejected.
- Changing `utl_hash_contains` or redesigning key ownership would exceed this
  batch.
- The project has no dedicated hash unit-test source or targeted hash test
  command. Creating new hash test infrastructure was not necessary for this
  source-level qualifier change; all current consumers were rebuilt and the
  complete existing suite was run twice.

### Final choice and rationale

- Change only the public declaration and implementation definition of
  `utl_hash_remove` from `void *key` to `const void *key`.
- Pass `rel_path` directly from `vault_note_delete`.
- Preserve node unlinking, error returns, stored-key representation, and
  ownership behavior.

This is a source-level const-correctness correction. The C symbol, argument
count, pointer representation, calling convention, and runtime lookup/delete
rules remain unchanged. External consumers should recompile against the
updated declaration.

### Actual changes

- Updated `utl_hash_remove` declaration and definition to accept
  `const void *key`.
- Removed `(void *)rel_path` and its cast-away-const comment from
  `vault_note_delete`.
- Added no replacement cast, key copy, warning suppression, API expansion, or
  test change.

### Commands and results

- Initial `git status --short`: confirmed and preserved all existing changes.
- Two pre-change `scons test` runs: each exited 0 and actually ran all
  `22/22` passing tests.
- Pre-change `scons`: exited 0; the build was up to date.
- Pre-change `git diff --check`: exited 0.
- First post-change `scons test`: exited 0; rebuilt vault, search, and hash
  objects without warnings; all `22/22` tests passed.
- Second post-change `scons test`: exited 0; the test action ran again without
  recompilation and all `22/22` tests passed.
- Post-change `scons`: exited 0 and linked `app_example`, `vault-cli`, and
  `vaultd`.
- Standalone production-options compile of `vault.c`: exited 0 with empty
  diagnostic output.
- `git diff --check`: exited 0.
- The hash diff shows the accumulated preceding get/helper const changes plus
  this batch's remove declaration/definition changes.
- The vault diff shows the preceding backlink truncation fix plus this batch's
  removal of the explicit cast.
- `git diff -- tests` remains empty because `tests/` is untracked; no test file
  was changed in this batch.
- The required cast search reports only the const remove declaration,
  definition, and direct `utl_hash_remove(v->notes, rel_path)` call.

### Warnings and failures

- No test, build, standalone compile, or check failed.
- No format-truncation, discarded-const, incompatible-pointer-types, or new
  compiler warning appeared.
- No targeted hash test was available in the project.

### Discovered but unresolved issues

- `vault_note_delete` obtains `found` from the hash, then calls
  `note_free(found)` and `free(found)` before calling `utl_hash_remove`.
- The stored hash key points at `found->path`, so the subsequent remove
  comparison can read the stored key after its owning `note_t` has been freed.
  This is an existing lifetime/order risk independent of the caller's
  `rel_path`; the caller-provided query key itself is never freed.
- Correcting that order and adding vault delete behavior coverage were
  explicitly outside this const-only batch and were not attempted.

### Behavior still uncertain

- There is no current vault delete regression test proving successful index
  removal or detecting the stored-key lifetime risk.
- Stored keys and values remain caller-managed because the hash has no
  destructor callbacks.

### Next steps

- Design a separate, explicitly approved fix for the
  remove-before-free ordering in `vault_note_delete`, with a focused delete
  regression test.
- Keep any `utl_hash_contains` const review separate from that lifecycle fix.

## 2026-07-23 — Safe vault note deletion ordering

### Goal and scope

- Remove the potential stored-key use-after-free in `vault_note_delete` by
  unlinking the hash node before freeing the owning `note_t`.
- Add one public-API delete regression test, including close/reopen
  persistence verification.
- Do not modify hash APIs, hash implementation, note/search behavior,
  backlink logic, or existing tests.

### Key files read or modified

- Read: `src/vault/vault.c`, current vault tests and fixture helpers,
  `tests/test_main.c`, `tests/SConscript`, the SCons environment loading path,
  and existing test documentation.
- Modified: `src/vault/vault.c`, `tests/test_vault.c`,
  `tests/test_main.c`, `tests/README.md`, `docs/TESTING.md`, and this log.
- Not modified in this batch: `utl_hash.h`, `utl_hash.c`, note/search sources,
  vault public headers, the harness, or existing backlink tests.

### Existing behavior understood

- A notes hash node stores `note->path` as its key and the owning `note_t *` as
  its value.
- The old delete order freed the `note_t` before asking the hash to compare and
  unlink its node, leaving the stored key pointer dangling during comparison.
- `utl_hash_remove` only unlinks and frees `hash_node_t`; it does not free the
  stored key, value, or caller-provided query key.
- Filesystem unlink occurs before hash removal. Search removal and backlink
  rebuilding occur after successful hash removal.

### Options and tradeoffs considered

- Copying either path would conceal the lifetime error and was rejected.
- Making the hash own or destroy keys/values would change the hash API and
  ownership model and was outside scope.
- The minimal safe order is hash-node removal, then note resource release.
- If hash removal fails, retaining `found` prevents a live hash node from
  referencing freed storage. Because file unlink already occurred, this
  failure path can leave an in-memory note whose backing file is gone; search
  and backlinks remain unchanged, and the remove error is returned.

### Final choice and rationale

- Keep the existing file unlink before hash handling.
- Call `utl_hash_remove(v->notes, rel_path)` while `found->path` is valid.
- On remove failure, unlock and return the remove error without freeing
  `found` or changing search/backlinks.
- On success, call `note_free(found)`, free `found`, then retain the existing
  search removal and backlink rebuild order.

Normal successful deletion still returns `UTL_OK`, removes the file and
in-memory note, updates search, and rebuilds backlinks.

### Actual changes

- Reordered hash removal before `note_free(found)` and `free(found)`.
- Added explicit propagation of an unexpected hash remove error.
- Added `vault_delete_removes_note_safely` using two independent equal path
  arrays for create and delete.
- The test verifies `UTL_ERR_NOT_FOUND` immediately after deletion, closes and
  reopens the same unique temporary vault, verifies the note is still absent,
  and cleans the deleted file path and temporary root.
- Registered the test and updated stable testing documentation, increasing the
  suite from 22 to 23 tests.

### Commands and results

- Initial `git status --short`: confirmed and preserved the existing dirty
  worktree.
- Pre-change `scons test`: exited 0; all `22/22` tests passed.
- First post-change `scons test`: exited 0; rebuilt the changed test and vault
  sources without warnings; `vault_delete_removes_note_safely` passed and all
  `23/23` tests passed.
- Second post-change `scons test`: exited 0; no recompilation was needed, the
  test action still executed, and all `23/23` tests passed.
- `scons`: exited 0 and linked `app_example`, `vault-cli`, and `vaultd`.
- Standalone production-options compile of `vault.c`: exited 0 with empty
  diagnostic output.
- `git diff --check`: exited 0.
- `git diff -- src/vault/vault.c`: showed the accumulated earlier backlink
  fix and const-cast removal plus this batch's remove-before-free order and
  failure return.
- `git diff -- tests`: remained empty because `tests/` is untracked; the
  changed test files were inspected directly.
- The required order search showed `utl_hash_remove` at `vault.c:340`,
  followed by `note_free` and `free` at lines 346 and 347.

### AddressSanitizer verification

- Built `/tmp/vaultcode_tests_asan` from the complete test and production
  source sets using the project base flags plus `-fsanitize=address` and
  `-fno-omit-frame-pointer`; compilation exited 0 without warnings.
- Running with `detect_leaks=1` exited 1 before tests because LeakSanitizer
  reported that it cannot operate under the environment's ptrace execution.
  Therefore leak detection was not completed and is not claimed as passing.
- Re-running the same binary with only leak detection disabled
  (`detect_leaks=0`, `halt_on_error=1`) exited 0; all `23/23` tests passed with
  no AddressSanitizer use-after-free, double-free, or other address error.

### Warnings and failures

- No regular test, production build, standalone compile, or whitespace check
  failed.
- No compiler warning was emitted.
- The leak-enabled sanitizer run was unavailable due to the ptrace environment;
  the address-safety run completed successfully.

### Discovered but unresolved issues

- An unexpected hash remove failure after successful filesystem unlink leaves
  partial state: the note remains valid and indexed in memory while its file
  may already be absent. This preserves memory safety and propagates the actual
  remove error, but no fault-injection test exists for this otherwise
  unreachable-under-normal-locking path.
- LeakSanitizer still needs to be run in an environment not controlled through
  ptrace to complete leak verification.

### Behavior still uncertain

- Filesystem unlink errors remain ignored by the existing implementation and
  were not changed or tested.
- Search removal is currently a stub; this test defines only public delete/get
  and persistence behavior, not broader search-index semantics.

### Next steps

- Run the ASan binary with leak detection enabled in a non-ptrace environment.
- Keep filesystem error propagation, search consistency, and delete fault
  injection in separate explicitly designed tasks.
