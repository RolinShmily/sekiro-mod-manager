# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). This file is a human-readable
summary; `git log` remains the authoritative record.

## [0.2.0] - 2026-09-22

### Added

- Community health files: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
  `SECURITY.md`, `CHANGELOG.md`, `.editorconfig`, GitHub issue forms, a pull-request template, a
  Dependabot configuration, and `rust-toolchain.toml`.
- `LICENSING.md`, which now carries the full MIT scope and third-party notices.
- CI now enforces `cargo fmt --all -- --check`, runs
  `cargo clippy --workspace --all-targets -- -D warnings`, and installs with `--frozen-lockfile`.

### Changed

- `README.md` / `README.zh-CN.md` slimmed down: the licensing analysis moved to `LICENSING.md`, and
  the architecture tree and CLI reference were corrected against the actual source.
- Rust sources normalized with `rustfmt` and 14 Clippy lints resolved; no behavior change.
- Repository hygiene: dropped the stale duplicate `apps/smm-desktop/pnpm-lock.yaml` and stopped
  tracking the generated `tsconfig.node.tsbuildinfo` and `src-tauri/gen/schemas/`.

### Fixed

- `.gitattributes` never applied its `LICENSE text eol=lf` rule — the rule had been concatenated onto
  a comment line, so license texts could be rewritten to CRLF on Windows checkouts.
- `scripts/build-installer.ps1` printed an empty path at the end of a successful packaging run; it
  referenced an undefined `$setupExe` instead of `$distSetupExe`.
- The README documented CLI flags that do not exist (`--staging-dir`, and `--game-dir` for
  `deploy` / `restore`; the real flags are `--staging` and `--target`).
- The README credited the UI to the Maple Mono typeface; SMM ships subset Inter, JetBrains Mono and
  Noto Sans SC.

## [0.1.8] - 2026-09-22

### Changed

- Licensing is consolidated: `licenses/` holds only the verbatim upstream texts, and the MIT scope
  plus third-party notices live in [LICENSING.md](LICENSING.md).

### Fixed

- Release notes are generated with PowerShell 7 (`pwsh`); Windows PowerShell 5.1 read the script
  using the ANSI codepage and mangled every non-ASCII character.

## [0.1.7] - 2026-09-22

### Added

- Declare the MIT scope explicitly and ship the third-party license texts in every release.

### Changed

- **Breaking:** ModEngine is no longer redistributed. SMM detects an installed `dinput8.dll` and
  patches `modengine.ini`, but requires the user to supply the payload themselves.

## [0.1.6] - 2026-09-20

### Added

- Self-hosted subset fonts (Inter, JetBrains Mono, Noto Sans SC) with a build-time subsetting script
  and a CI check that every artifact is present and non-empty.

### Fixed

- Restore `http-body-util` to `0.1.5` in `Cargo.lock`; an over-eager bump broke dependency
  resolution in CI.

## [0.1.5] - 2026-09-13

### Added

- Support `audio/` and `voice/` mod signatures and object prop mods.

### Fixed

- Sanitize quoted paths supplied on the command line.

## [0.1.4] - 2026-09-13

### Fixed

- Normalizer supports UI subdirectories (`hi/`, `font/`) nested under `menu/`, ignores Yabber
  unpack artifacts, and detects multi-folder mod signatures reliably.

## [0.1.3] - 2026-09-13

### Added

- Multi-file merged import, mod activation presets, and multiple mod-list view modes.
- Native file picker plus shortcuts to launch the game and open its directory.

### Fixed

- Engine diagnosis and category layout; bump the embedded ModEngine payload.

## [0.1.2] - 2026-09-13

### Added

- Unified mod source URL handling with 3DM and GameBanana recognition.

### Fixed

- Open external links in the system browser.
- Importer supports `sfx/` canonical directories, loose `texbnd`/`vfx` signatures, and recursive
  extraction of nested archives.

## [0.1.1] - 2026-09-12

### Added

- Native file picker, RAR support, ModEngine integration, and a redesigned confirmation dialog.
- Source URL provenance, `.smmpack` modpack bundling, and the `setup.exe` packaging pipeline.

## [0.1.0] - 2026-09-12

### Added

- Initial Rust workspace with `smm-core` (engine) and `smm-cli` (`smm`).
- Tauri v2 + React 18 + Tailwind CSS desktop GUI with a vector icon system.
- GitHub Actions workflows for CI and tag-triggered releases.
- MIT license, `.gitignore`, and bilingual documentation.

[Unreleased]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.8...v0.2.0
[0.1.8]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.7...v0.1.8
[0.1.7]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.6...v0.1.7
[0.1.6]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.5...v0.1.6
[0.1.5]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/RolinShmily/sekiro-mod-manager/releases/tag/v0.1.0
