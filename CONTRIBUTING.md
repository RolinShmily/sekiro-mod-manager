# Contributing to Sekiro Mod Manager (SMM)

Thanks for helping. This page is the short version; the detailed rules live in [`docs/`](docs/README.md).

By participating you agree to the [Code of Conduct](CODE_OF_CONDUCT.md). Report security issues
privately per [SECURITY.md](SECURITY.md), never in a public issue.

---

## 1. Get it building

Requirements: Windows 10/11 x64, Visual Studio 2022 (MSVC v143), CMake 3.25+, Qt 6.11.2 and
Python 3.9+ (skill packaging + the HuskarUI patch script run at configure time).

```bash
git clone --recurse-submodules https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager
cmake --preset msvc-x64-debug && cmake --build --preset build-debug
```

`msvc-x64-release` for the optimised build, `core-only` when no Qt is installed.
Presets, submodule recovery and troubleshooting: [`docs/development/build.md`](docs/development/build.md).

## 2. Before you open a PR

```bash
# headless regression suites (core rules, CoreService adapter, update feed)
cmake --preset msvc-x64-release -DSMM_BUILD_TESTS=ON && cmake --build --preset build-release
ctest --test-dir build/msvc-x64-release --output-on-failure
```

What each suite covers, how to add one, and the manual GUI checklist:
[`docs/development/testing.md`](docs/development/testing.md).

## 3. Rules that are easy to get wrong

- **Respect the layer boundaries.** `smm_core` never includes Qt, never prints, and never knows
  about the GUI; business rules belong there, not in QML or the GUI backend.
  See [`docs/architecture/layers.md`](docs/architecture/layers.md).
- **Use design tokens.** QML takes colours, radii and durations from `HusTheme.Primary`, never
  hardcoded hex; animations are gated on `HusTheme.animationEnabled`.
  See [`docs/design/tokens.md`](docs/design/tokens.md).
- **Keep contracts stable.** CLI JSON field names, `ErrorCode` strings and on-disk file schemas
  are consumed by the GUI and by scripts: [`docs/reference/errors.md`](docs/reference/errors.md),
  [`docs/reference/file-formats.md`](docs/reference/file-formats.md).
- **Update docs in the same commit** as the behaviour you changed.
- Coding style (C++/QML/CMake): [`docs/development/code-style.md`](docs/development/code-style.md).

## 4. Commits and pull requests

- Conventional Commits (`feat(gui): …`, `fix(ci): …`, `refactor(core): …`), one concern per
  commit; stage files explicitly rather than sweeping the tree.
- PRs must keep `ci.yml` green (build + `ctest`) and describe user-visible changes; website-only
  changes are covered by `website.yml`. Details: [`docs/development/workflow.md`](docs/development/workflow.md).
- Contributions are submitted under the project's [MIT License](LICENSE).
