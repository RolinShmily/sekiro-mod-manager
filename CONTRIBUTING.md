# Contributing to Sekiro Mod Manager

Thanks for helping improve Sekiro Mod Manager. For full build, testing, and release details, see [`docs/development.md`](docs/development.md).

## 1. Local Setup

Prerequisites: Windows 10/11 x64, Visual Studio 2022 (MSVC v143), CMake 3.21+, Ninja, Qt 6.8+ (or 6.11), and Python 3.9+ (for `scripts/patch_huskarui.py`). Website and Worker work requires Node.js 22+ and `pnpm`.

```bash
git clone --recurse-submodules https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager
cmake --preset msvc-x64-debug -DSMM_BUILD_TESTS=ON
cmake --build --preset build-debug
```

Without Qt (core library & unit tests only):

```bash
cmake --preset core-only
cmake --build --preset build-core
```

## 2. Verification Before Opening a PR

```bash
# C++ core + headless GUI regression suites
ctest --test-dir build/msvc-x64-debug --output-on-failure

# Worker release proxy + website download parser tests & typecheck
pnpm install --frozen-lockfile --ignore-scripts
pnpm test
pnpm lint
```

## 3. Architectural Guardrails

- **Respect layer boundaries**: `src/core/` is pure C++17 (flat headers/sources, no Qt, no stdout/stderr). Business rules live in `src/core/`, not QML or GUI controllers. See [`docs/architecture.md`](docs/architecture.md).
- **Use theme tokens**: QML reads colors and radii from `HusTheme.Primary.*` instead of hardcoded hex values, and keeps UI transitions disabled except the optional preview cross-fade. See [`docs/design.md`](docs/design.md).
- **Keep on-disk & error contracts stable**: Do not rename `ErrorCode` strings or break `.smm_mod.json` / `.smmpack` / `.smm_manifest.json` backward compatibility. See [`docs/reference.md`](docs/reference.md).
- **Never redistribute third-party mods or ModEngine**: Do not commit `dinput8.dll` or third-party mod assets into this repository.

## 4. Commits & Pull Requests

- Use Conventional Commits (`feat(gui): ...`, `fix(core): ...`, `docs: ...`, `ci: ...`), keeping one logical concern per commit.
- Stage modified files explicitly (`git add <file>`).
- Keep `.github/workflows/test.yml` green and update relevant docs in the same PR.
- All contributions are licensed under the project's [MIT License](LICENSE).
