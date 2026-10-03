# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). This file is a human-readable
summary; `git log` remains the authoritative record.

## [0.3.3] - 2026-10-03

### Added

- **Standalone `sekiro.exe` Direct Launch**: Decoupled game launch from Steam protocol (`steam://rungameid/814380`). SMM now detects and prioritizes direct process execution (`QProcess::startDetached`) with the game root working directory for offline, standalone, or DRM-free installations.
- **Dynamic Telemetry & Manifest-Aware Stats**: Deployment metrics (linked files and disk space saved) now initialize to zero (`0` files, `0 B` saved) instead of mock defaults, accurately reflect live `.smm_manifest.json` data, and instantly zero out on environment restore.
- **HuskarUI-First Component Alignment**: Upgraded batch selection checkboxes to `HusCheckBox`, converted mod description inputs to auto-sizing `HusTextArea`, added popconfirm dialogs (`HusPopconfirm`) for mod pack preset deletion, and improved Chinese typography rendering (DemiBold 600 weight with DirectWrite NativeRendering).

### Changed

- **Standardized Metadata (`.smm_mod.json`)**: Migrated legacy `mod.json` staging metadata exclusively to `.smm_mod.json`, automatically purging old files upon normalization.
- **Archive Staging Backup (`.smm_source`)**: Preserved imported `.zip` / `.7z` archives in `.smm_source` inside each mod staging directory for lossless archive re-export.
- **Clean Export Pipeline**: Mod packs and standalone mod exports exclude internal `.smm_source` backup archives by default (`ExportOptions::include_source = false`), ensuring compact, distribution-ready output packages.
- **Custom Image Stem Normalization**: Preserved original image stem names for preview assets while automatically transcoding WebP/BMP formats to single standalone PNG files.

## [0.3.2] - 2026-10-02

### Added

- **Real-Time Preview Image Conversion & Healing**: Added native Windows Imaging Component (WIC) image normalizer in `smm_core`. Automatically converts WebP / corrupt image formats to standard PNG on upload and auto-heals legacy mod preview images. Integrated cache-busting revision tracking in QML for instant real-time UI feedback.
- **Unequip Mod Pack Preset**: Added dedicated "Unequip Mod Pack" option in the preset view to disengage active preset locks and return to freeform mod management.
- **Equipped Mod Pack Filter**: Added a toggle button in the mod management toolbar to view only mods belonging to the active mod pack, with $O(1)$ memory lookup.
- **Mod Metadata Full Editing**: Added full editing for Mod ID, display name, author, and description in `ModDrawer` with automatic safe folder renaming in staging.
- **Domain-Matched Source Icons**: Added automatic platform icon recognition for Nexus Mods, GameBanana, 3DM Mods, Bilibili, and GitHub with official brand assets.
- **Streamlined Workflow Layout**: Relocated "Save Setup as Mod Pack" directly into the Armoury view toolbar next to batch operations.
- **50% Package Size Slimming**: Pruned 20MB software rasterizer (`opengl32sw.dll`), 15MB redundant QtQuick Controls styles (FluentWinUI3, Imagine, Material, Universal, Fusion), and unreferenced embedded assets; reduced portable bundle from 135MB down to ~59MB (Release ZIP down to ~23MB).
- **QML Image Memory Downsampling (`sourceSize`)**: Constrained decoded bitmap dimensions in `ModCard` and `LaunchHubView`, avoiding full 1080p/2K image uncompressed RAM allocation and saving up to 96% per-card image memory.

## [0.3.1] - 2026-10-02

### Added

- **Batch Mod Management**: Added multi-select batch management in the Armoury view with select-all, batch enable/disable, and batch delete with confirmation modal.
- **Mod Pack Preset Import Sync**: Automatically registers imported `.smmpack` archives into `.smm_presets.json` and emits `refreshPresets()` for instantaneous preset list presentation.
- **Bilingual Batch i18n**: Completed full English translations for all batch management actions and dialogs.

### Changed

- **Reliable Desktop Update Experience**: Refactored the update system into a clean version notification and one-click installer / release page launcher, eliminating fragile in-process file overwriting, Windows process locks, and installer version desynchronization.

### Fixed

- **Settings Persistence & Mod Reload**: Resolved staging directory reverting to default on uncreated paths; added automatic recursive folder creation via `fs::create_directories` and `settings.sync()`.
- **Atomic Settings Confirmation**: Deferred language switching and directory changes to apply atomically on "Save Settings" click instead of taking immediate effect.
- **Settings Scoped ID Resolution**: Fixed `ReferenceError: stagingDirInput is not defined` by managing pending states on the root modal across independent delegates.
- **Responsive Toolbar Layout**: Restructured mod management toolbar into a two-row responsive layout, preventing right-edge truncation of view toggles and action buttons on narrow windows.
- **Mod Pack Card Visual Overhaul**: Redesigned preset cards to 156px compact cards with mod count tags, status badges, elegant fallback descriptions, and refined primary action buttons.
- **Unicode/Code Page Immunity**: Replaced ANSI code page conversions with Win32 CP_UTF8 conversions via `path_to_utf8` and `utf8_to_path`, completely resolving 'No mapping for the Unicode character exists in the target multi-byte code page' and restoring full compatibility with legacy mod repositories.
- **Path Normalization & Multi-Archive Batch Import**: Unified directory path separators to native Windows backslashes with URL percent-encoding decoding; enabled multi-file selection in `FileDialog` for batch archive importing.

## [0.3.0] - 2026-10-02

### Added

- **Architectural Rewrite in C++17 & Qt 6 / QML**: Replaced legacy Rust/Tauri stack with high-performance native C++17 libraries and Qt 6 QML desktop GUI powered by HuskarUI.
- **In-Process Core Integration**: `smm_gui` links `smm_core` directly in-process for instantaneous UI response and zero subprocess overhead.
- **Sumi-e & Modern Tactile Aesthetics**: High-contrast, restrained dark/light theme styling with custom calligraphy window icons and watermark artwork.
- **Bilingual Internationalization (i18n)**: Seamless dynamic language switching between Simplified Chinese (`zh-CN`) and English (`en-US`) with 150+ localized keys.
- **Full Environment Health Doctor**: Interactive diagnostics for `sekiro.exe`, ModEngine `dinput8.dll`, `modengine.ini`, and NTFS cross-volume link viability with automated repair.
- **Fine-Grained Asset Management**: Per-asset enable/disable toggles inside the mod drawer.
- **Immersive Card Backdrops**: Visual blurred background artwork for mod cards with automatic `.smmpack` packaging.
- **Multi-Depth Archive Parsing**: Automatic 6-level recursive nested zip extraction and loose file heuristic categorization.

### Changed

- Transitioned build system to standard CMake 3.25+ with Ninja and Visual Studio 2022 presets.
- Preserved 100% backward compatibility with v0.2.0 mod staging directories and portable `.smmpack` mod packs.
- Cleaned and modernized repository root: purged obsolete Node/Rust dependencies.

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
