# Engine review scope

The camera implementation and its automated tests are under `src/`, including
their CMake source lists and English settings strings. Review them independently
of the fork's downloads, promotional media and test-map tooling:

```text
git diff upstream/master -- src/
git diff --binary upstream/master -- src/ > engine-review.patch
```

Use a fetched upstream revision as the comparison base. The commands include
tracked local changes. For a published build, run them from a clean checkout of
that build's source revision. Keep exported patches outside the source tree or
under an ignored build directory.

The following files serve the dedicated fork and are outside that engine diff:

- `.github/prepare-fork-package.py`, `.github/verify-fork-packages.py` and fork
  publication changes in `.github/workflows/`.
- The fork-only packaging changes in `cmake/InstallAndPackage.cmake`.
- The homepage, download instructions and animated media.
- `docs/validation/` fixtures, capture tools and historical validation records.
- The development handoff and TODO documents.

Keep these files in this repository for users and maintainers. Engine build and
test registration stays with the source it covers. Compiler and renderer fixes
also belong in the engine diff when the camera implementation depends on them.

The fork enables the features by default in its settings table and tests that
choice. That is a deliberate fork policy, separate from the camera mechanics.
Reviewing an engine diff does not change the
[submission eligibility restrictions](upstream-readiness.md#submission-eligibility).
