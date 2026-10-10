# Upstream requirements and fork validation

Reviewed 10 October 2026 against OpenTTD's contribution guide, coding style and checked-in CI workflows.

## Submission eligibility

[OpenTTD's AI policy](https://github.com/OpenTTD/OpenTTD/blob/master/CONTRIBUTING.md#use-of-ai) prohibits submissions containing AI-generated code, including entire generated lines, and AI-written issues or PR descriptions. This fork contains such code. Passing tests or reviewing the code does not remove that restriction. This work is therefore not eligible for an upstream PR under the current policy. Do not conceal its provenance. Development and validation here are for the private fork.

The [contribution guide](https://github.com/OpenTTD/OpenTTD/blob/master/CONTRIBUTING.md#pull-requests) also asks contributors to discuss significant features first, keep each PR focused, use logical commits and a topic branch based on current upstream master, and accept the project's GPL v2 license and contributor privacy terms. These are submission requirements, not evidence that maintainers have agreed to accept smooth zoom.

## Technical requirements and evidence

| Requirement | Fork status |
| --- | --- |
| Complete build followed by CTest, with a 120-second per-test timeout | Previously only the game and unit executable were built. Regression fixtures are now staged and all four upstream AI/GS regression tests pass. The local runner builds all default targets before CTest. |
| Meaningful automated tests | Smooth rendering, palette tracking, pixel phase, controls, flight paths and target fitting are registered in the existing Catch suite; 140 cases, 68,807 assertions passed before this audit. |
| Source file descriptions | Run the upstream checker with explicit forward-slash paths on Windows; its default directory walker assumes Linux separators. |
| Source/build inventory | Registered all six new core headers in `src/core/CMakeLists.txt`; headers were previously compilable through includes but absent from the project inventory. |
| English strings and translations | New settings use English source strings. Translation changes belong in [Eints](https://github.com/OpenTTD/OpenTTD/blob/master/docs/eints.md), rather than invented translations. The unused-string checker needs GNU C++ preprocessing. Its unmodified Windows path handling gives false cargo-string errors; run it on Linux. |
| Source checks on private master | Added `fork-source-checks.yml` for pushes and manual runs. Upstream source-description/string workflows otherwise run only on PRs. Existing build CI already runs on master pushes. |
| Commit formatting | [Coding style](https://github.com/OpenTTD/OpenTTD/blob/master/CODINGSTYLE.md#commit-message) requires prefixes such as `Fix:`, `Feature:`, `Codefix:` and `Doc:`. Earlier exploratory fork commits do not consistently comply. Future commits use the format; published private history has not been rewritten. |
| Documentation validation | Upstream compares Doxygen warnings against the PR base, including source, AI and GS docs. That comparison has not been run locally; file descriptions are a separate check. |
| Cross-platform CI | [Build matrix](https://github.com/OpenTTD/OpenTTD/blob/master/.github/workflows/ci-build.yml): Linux Clang/GCC/SDL2 and dedicated/no-PCH, macOS arm64 Debug/Release, Windows x86/x64, Emscripten. Local evidence is Windows x64 only. Other jobs must pass before claiming platform coverage. |
| Compatibility and performance | See [graphics coverage](zoom-coverage.md), including three graphics sets, renderer variants, save round trips and fractional-pan timings. GUI evidence supplements CTest; it cannot replace platform builds. |

## Repeat the local check

Configure the build as described in [COMPILING.md](../../COMPILING.md), provide an installed OpenGFX base set, and close any prototype using that build's executable before linking. Then run:

```powershell
py -3 docs/validation/run-upstream-checks.py --build-dir C:/ottd-build --cmake 'C:/Program Files/CMake/bin/cmake.exe'
```

The runner checks source descriptions, builds all default targets (including regression fixtures), and runs the complete CTest suite. It exits on any failure. It does not install dependencies or silently skip regression tests. On Linux, additionally run `python3 .github/unused-strings.py` with GNU C++ installed, as the private source-check workflow does.

## Remaining verification

- After the repository became public, the rerun started normally. Linux source-description/string checks and the rebase-label check passed. First-pass Emscripten, Linux Clang and Windows x86 passed; the remaining client builds and CodeQL were still running when the fixes below were prepared.
- First CI feedback: macOS Debug/Release cannot use floating-point `std::from_chars` at the supported deployment target. Replaced the console parser with a strict decimal, classic-locale stream parser and tests for malformed input and comma-decimal global locales. Dedicated builds do not link real blitters, so renderer tests now follow `NOT OPTION_DEDICATED`; pure camera/controls tests remain enabled. GPU renderer tests also follow `WITH_OPENGL`.
- The shared vcpkg action uses a read/write GitHub Packages cache. Added explicit package-write permissions to build and CodeQL workflows after the logs showed nonfatal 403 cache-upload failures.
- Local validation after these fixes: all **146 CTest tests passed** (25.18 seconds), including all four upstream regressions. Remote confirmation is pending.
- The 10 October private Actions runs did not start: GitHub reported failed recent account payments or an insufficient spending limit. [Source-check run](https://github.com/Deku-shrub/OpenTTD/actions/runs/38082645949) and [build-matrix run](https://github.com/Deku-shrub/OpenTTD/actions/runs/38082646432) have no executed build steps. Account billing must be resolved before remote validation can provide evidence.
- Local audit result: source-description check passed, full default-target Release build succeeded, and all **144 CTest tests passed**, including the four AI/GS regression suites (18.73 seconds). This closes the earlier missing-fixture failure.
- Confirm results of the private remote build matrix and new Linux source checks; queued or configured jobs are not passing evidence.
- Compare Doxygen warning sets with the unmodified upstream base.
- Complete multiplayer/client compatibility testing and remaining platform-specific GUI checks listed in the [TODO](../smooth_zoom_todo.md).
- Keep fork implementation history and large demonstration assets separate from any future independently authored, policy-compliant contribution. No upstream issue or PR has been submitted by this audit.
