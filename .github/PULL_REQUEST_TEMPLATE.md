## Summary

<!-- What does this change do, and why? Link the related issue: "Closes #123". -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / internal cleanup
- [ ] Documentation
- [ ] Build, CI, or packaging
- [ ] **Breaking change** (CLI JSON field, `ErrorCode` name, or on-disk file name)

## Verification

<!-- How did you confirm this works? Paste the commands you ran and the relevant output. -->

Configure and build (see `docs/development/build.md`):

- [ ] `cmake --preset msvc-x64-debug` then `cmake --build --preset build-debug`
- [ ] Reconfigured with `-DSMM_BUILD_TESTS=ON` and ran `ctest --test-dir build/msvc-x64-debug --output-on-failure`
- [ ] `./build/msvc-x64-debug/src/cli/smm.exe doctor --json`
- [ ] `qmllint -I src/gui/third_party/HuskarUI/src/imports -I src/gui/qml src/gui/qml/Main.qml src/gui/qml/components/*.qml src/gui/qml/views/*.qml` (QML changes)
- [ ] Manually exercised the affected flow (import → plan → deploy → restore, or the UI path)

## Checklist

- [ ] The change is focused; unrelated refactors are in separate commits.
- [ ] Business rules live in `smm_core` (`src/core/`), so the CLI and GUI stay in sync.
- [ ] Tests were added or updated for behavior changes (`src/core/tests/`, `src/gui/tests/`).
- [ ] New QML files and resources are registered in `src/gui/CMakeLists.txt`.
- [ ] `README.md` **and** `README.zh-CN.md` were updated together (if user-facing text changed).
- [ ] Docs updated in the same commit: `docs/reference/cli.md`, `docs/reference/json-contract.md`, `docs/reference/errors.md`, `docs/reference/file-formats.md` (whichever applies).
- [ ] `LICENSING.md` / `THIRD_PARTY_NOTICES.md` updated (if a redistributed dependency changed).
- [ ] No ModEngine binaries, game assets, or third-party mod content were added.
- [ ] No build output was committed (`build/`, `dist/`, `*.tsbuildinfo`, `*.obj`).
