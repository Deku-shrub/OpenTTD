# Upstream requirements and fork validation

Reviewed 10 October 2026 against OpenTTD's contribution guide, coding style and checked-in CI workflows.

## Repository lineage and comparisons

`Deku-shrub/OpenTTD` is now a native public GitHub fork of `OpenTTD/OpenTTD`.
All three existing branches were restored without rewriting their commit history.
`upstream-base` records the original base `fab8089e6bed2be570b6b0a2cb9f19a5db16a7da`;
`upstream-master` records the setup checkpoint `d48d693d7ff09b643846e23a5f033cf3582ab27f`.
The original standalone repository remains at
[OpenTTD-standalone-backup](https://github.com/Deku-shrub/OpenTTD-standalone-backup),
including its historical CI runs. Repository-level run history is not copied to
the native fork.

- [Diff from original base](https://github.com/Deku-shrub/OpenTTD/compare/upstream-base...master)
- [Comparison against current upstream](https://github.com/OpenTTD/OpenTTD/compare/master...Deku-shrub:master)

Both comparison endpoints were verified. At migration, the fork was one commit
behind current upstream (a Lithuanian translation fix), with the original base
as their common ancestor. Migration did not silently merge or rebase new upstream
changes, and no upstream pull request was created.

## 11 October 2026 follow-up

- Public `master` revision `c4dd39832b` passed [all eight platform jobs](https://github.com/Deku-shrub/OpenTTD/actions/runs/38121763743). The [nightly checks](https://github.com/Deku-shrub/OpenTTD/actions/runs/38107874054) also passed. The earlier native-fork [CodeQL analysis](https://github.com/Deku-shrub/OpenTTD/actions/runs/38090036201) passed; an analysis run succeeding does not mean its alert list is empty.
- Reviewed all 299 open CodeQL alerts against lines changed from the original upstream base. Nine findings overlap added lines, all `cpp/equality-on-floats`: exact copied scale/state values, integer-valued destination coordinates, and unchanged-target checks. These comparisons deliberately detect identical state; approximate equality would incorrectly reuse a sampling phase or ignore a distinct target. No new memory-safety/security finding overlapped added lines. Existing upstream/vendor alerts remain open; this review does not claim the whole engine is free of defects.
- Removed obsolete motion-quality flags and their redundant sampling-selection methods. Viewports already use the same centre sampling while moving and at rest. Shared vehicle arrival fitting now serves both initial navigation and moving-target updates.
- The first documentation comparison found 228 new warning blocks. Added API, member, parameter and return documentation. The next comparison reduced this to one missing shared-helper declaration comment; that declaration is now documented and the [final comparison passed](https://github.com/Deku-shrub/OpenTTD/actions/runs/38122911730), with no new source, AI or Game Script documentation warnings.
- Camera cleanup validation: complete Windows x64 Release rebuild, source-description check and all **146 CTest tests passed** (22.89 seconds), including the four upstream regression suites. The rebuilt software, SSE and GPU renderers pass stable-rest and copied-pan versus full-redraw checks at 130%, 84% and 66?%. The Windows ZIP was extracted and launched using only its bundled graphics and language files; smooth zoom and flights were enabled for its fresh profile.
- [Review-branch CI](https://github.com/Deku-shrub/OpenTTD/actions/runs/38123425250) passed all eight platform jobs and produced all four package artifacts. Publication will be verified after promotion to master. Windows x64/x86 portable ZIPs, Ubuntu 24.04 x64 DEB and Apple Silicon macOS DMG include checksum-verified OpenGFX 8.0. Packages are built after their tests pass; publication waits for the entire platform matrix. Each release records its source revision and checksums. Manual review-branch builds produce artifacts without publishing releases.
- The standalone backup repository can be deleted if its historical CI run logs are no longer needed. Its branches are preserved in this native fork; it had no issues, releases, stars or forks at migration. No repository deletion was performed.

## Submission eligibility

[OpenTTD's AI policy](https://github.com/OpenTTD/OpenTTD/blob/master/CONTRIBUTING.md#use-of-ai) prohibits submissions containing AI-generated code, including entire generated lines, and AI-written issues or PR descriptions. This fork contains such code. Passing tests or reviewing the code does not remove that restriction. This work is therefore not eligible for an upstream PR under the current policy. Do not conceal its provenance. Development and validation here are for this public fork.

The [contribution guide](https://github.com/OpenTTD/OpenTTD/blob/master/CONTRIBUTING.md#pull-requests) also asks contributors to discuss significant features first, keep each PR focused, use logical commits and a topic branch based on current upstream master, and accept the project's GPL v2 license and contributor privacy terms. These are submission requirements, not evidence that maintainers have agreed to accept smooth zoom.

## Technical requirements and evidence

| Requirement | Fork status |
| --- | --- |
| Complete build followed by CTest, with a 120-second per-test timeout | Previously only the game and unit executable were built. Regression fixtures are now staged and all four upstream AI/GS regression tests pass. The local runner builds all default targets before CTest. |
| Meaningful automated tests | Smooth rendering, palette tracking, pixel phase, controls, flight paths and target fitting are registered in the existing Catch suite; 140 cases, 68,807 assertions passed before this audit. |
| Source file descriptions | Run the upstream checker with explicit forward-slash paths on Windows; its default directory walker assumes Linux separators. |
| Source/build inventory | Registered all six new core headers in `src/core/CMakeLists.txt`; headers were previously compilable through includes but absent from the project inventory. |
| English strings and translations | New settings use English source strings. Translation changes belong in [Eints](https://github.com/OpenTTD/OpenTTD/blob/master/docs/eints.md), rather than invented translations. The unused-string checker needs GNU C++ preprocessing. Its unmodified Windows path handling gives false cargo-string errors; run it on Linux. |
| Source checks on fork master | Added `fork-source-checks.yml` for pushes and manual runs. Upstream source-description/string workflows otherwise run only on PRs. Existing build CI already runs on master pushes. |
| Commit formatting | [Coding style](https://github.com/OpenTTD/OpenTTD/blob/master/CODINGSTYLE.md#commit-message) requires prefixes such as `Fix:`, `Feature:`, `Codefix:` and `Doc:`. Earlier exploratory fork commits do not consistently comply. Future commits use the format; published private history has not been rewritten. |
| Documentation validation | Upstream compares Doxygen warnings against the PR base, including source, AI and GS docs. The fork comparison passed with no new warnings against the original upstream base; file descriptions are a separate check. |
| Cross-platform CI | [Build matrix](https://github.com/OpenTTD/OpenTTD/blob/master/.github/workflows/ci-build.yml): Linux Clang/GCC/SDL2 and dedicated/no-PCH, macOS arm64 Debug/Release, Windows x86/x64, Emscripten. All eight jobs passed for published revision `c4dd39832b`. Platform GUI behaviour still needs independent runtime validation. |
| Compatibility and performance | See [graphics coverage](zoom-coverage.md), including three graphics sets, renderer variants, save round trips and fractional-pan timings. GUI evidence supplements CTest; it cannot replace platform builds. |

## Repeat the local check

Configure the build as described in [COMPILING.md](../../COMPILING.md), provide an installed OpenGFX base set, and close any prototype using that build's executable before linking. Then run:

```powershell
py -3 docs/validation/run-upstream-checks.py --build-dir C:/ottd-build --cmake 'C:/Program Files/CMake/bin/cmake.exe'
```

The runner checks source descriptions, builds all default targets (including regression fixtures), and runs the complete CTest suite. It exits on any failure. It does not install dependencies or silently skip regression tests. On Linux, additionally run `python3 .github/unused-strings.py` with GNU C++ installed, as the fork source-check workflow does.

## Historical CI feedback and remaining verification

- After the repository became public, the rerun started normally. Linux source-description/string checks and the rebase-label check passed. First-pass Emscripten, Linux Clang and Windows x86 passed; the remaining client builds and CodeQL were still running when the fixes below were prepared.
- First CI feedback: macOS Debug/Release cannot use floating-point `std::from_chars` at the supported deployment target. Replaced the console parser with a strict decimal, classic-locale stream parser and tests for malformed input and comma-decimal global locales. Dedicated builds do not link real blitters, so renderer tests now follow `NOT OPTION_DEDICATED`; pure camera/controls tests remain enabled. GPU renderer tests also follow `WITH_OPENGL`.
- The shared vcpkg action uses a read/write GitHub Packages cache. Added explicit package-write permissions to build and CodeQL workflows after the logs showed nonfatal 403 cache-upload failures.
- Local validation after these fixes: all **146 CTest tests passed** (25.18 seconds), including all four upstream regressions. Remote confirmation subsequently passed on all eight platforms.
- The initial 10 October private Actions runs did not start: GitHub reported failed recent account payments or an insufficient spending limit. [Source-check run](https://github.com/Deku-shrub/OpenTTD-standalone-backup/actions/runs/38082645949) and [build-matrix run](https://github.com/Deku-shrub/OpenTTD-standalone-backup/actions/runs/38082646432) have no executed build steps. Making the repository public subsequently allowed standard hosted jobs to start.
- Local audit result: source-description check passed, full default-target Release build succeeded, and all **144 CTest tests passed**, including the four AI/GS regression suites (18.73 seconds). This closes the earlier missing-fixture failure.
- Completed: the public build matrix, source checks and nightly checks passed; queued or configured jobs were not counted as passing evidence.
- Completed: compare Doxygen warning sets with the unmodified upstream base; no new warnings remain.
- Complete multiplayer/client compatibility testing and remaining platform-specific GUI checks listed in the [TODO](../smooth_zoom_todo.md).
- Keep fork implementation history and large demonstration assets separate from any future independently authored, policy-compliant contribution. No upstream issue or PR has been submitted by this audit.
