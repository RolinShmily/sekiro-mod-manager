<div align="center">

# Sekiro Mod Manager (SMM)

**High-Performance, Zero-Disk-Copy Desktop Mod Manager & Automation Core for *Sekiro: Shadows Die Twice***

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Rust](https://img.shields.io/badge/Rust-1.80%2B-orange.svg)](https://www.rust-lang.org/)
[![Tauri](https://img.shields.io/badge/Tauri-v2-24C8D8.svg)](https://tauri.app/)
[![React](https://img.shields.io/badge/React-18-61DAFB.svg)](https://react.dev/)
[![Tailwind CSS](https://img.shields.io/badge/Tailwind-v3-38B2AC.svg)](https://tailwindcss.com/)

[English](README.md) | [简体中文](README.zh-CN.md)

</div>

---

## Highlights

- ⚡ **Zero Disk Copy Deployment**: Utilizes Win32 native NTFS hard links (`CreateHardLinkW`) to project mods into the `mods/` directory in milliseconds, consuming zero additional disk space.
- 🛡️ **Pristine Rollback & Safety**: Every deployment is strictly tracked via `.smm_manifest.json`. One-click clean restore removes only managed hard links without touching baseline game assets.
- 🔍 **Intelligent Directory Normalization**: Strips arbitrary nested zip wrapper directories automatically, heuristics reposition loose signature files (`c0000.chrbnd.dcx`, `wp_a_0300.partsbnd.dcx`) into canonical FromSoftware game paths (`chr/`, `parts/`).
- ⚠️ **Three-Tier Conflict Engine**: Resolves file overrides by priority (`P:1 > P:10`), while flagging `[CRITICAL]` warnings on game balance collisions (`gameparam.parambnd.dcx`) and `[EXCLUSIVE SLOT]` on character/weapon skins.
- 📦 **Granular Source Tracking & Modpack Bundling**: Preserves origin URLs (NexusMods, GitHub, Netdisk), supports single-mod export with original archives, and exports/imports self-contained `.smmpack` modpacks.
- 🤖 **AI-Agent & Automation Friendly**: Includes both a modern desktop GUI and a standalone headless CLI (`smm.exe`), bundled directly in the installer for scripting and automated AI-agent workflows.
- 🎨 **Minimalist Japanese Hardware Aesthetic**: Styled strictly according to modern design tokens, stark white canvas, classic "隻狼" cinnabar gold seal logo, 100% vector SVG icons, and typography powered by Maple Mono.

---

## System Architecture

```text
sekiro-mods/
├── Cargo.toml                         # Workspace root configuration
├── LICENSE                            # MIT License
├── README.md                          # English documentation
├── README.zh-CN.md                    # Simplified Chinese documentation
├── DESIGN.md                          # UI/UX design specifications & design tokens
├── package.json                       # Scripts: build, package, test
├── pnpm-workspace.yaml                # Monorepo package workspace
├── crates/
│   ├── smm-core/                      # Core engine library
│   │   ├── src/
│   │   │   ├── conflict.rs            # Multi-level semantic conflict analyzer
│   │   │   ├── deploy.rs              # Winning asset deployment planner
│   │   │   ├── doctor.rs              # ModEngine diagnostic & auto-installer
│   │   │   ├── executor.rs            # Win32 NTFS hard link runner & rollback
│   │   │   ├── exporter.rs            # Modpack (.smmpack) & single export engine
│   │   │   ├── extractor.rs           # Multi-archive extractor (7z/zip/rar)
│   │   │   ├── importer.rs            # Normalizing archive importer & provenance
│   │   │   ├── loader.rs              # Mod directory discovery & metadata parser
│   │   │   ├── manager.rs             # Lifecycle, priority & metadata persistence
│   │   │   ├── normalizer.rs          # Heuristic directory normalization algorithm
│   │   │   └── types.rs               # Strongly typed data contracts
│   │   └── tests/                     # Automated integration test suites
│   └── smm-cli/                       # Standalone CLI tool (`smm`)
│       └── src/main.rs                # Headless command-line interface
├── apps/
│   └── smm-desktop/                   # Tauri v2 + React 18 + Tailwind CSS desktop GUI
│       ├── src/                       # React components (HeaderBar, ModList, ModDetails, etc.)
│       └── src-tauri/                 # Tauri v2 native bindings & IPC handlers
├── fixtures/                          # Benchmark mod fixtures for integration tests
├── dist-installer/                    # Windows installer output directory (setup.exe)
└── scripts/
    └── build-installer.ps1            # Automated one-click packaging pipeline
```

---

## Installation & Packaging Pipeline

### 1. Pre-built Windows Installer
Download the latest `setup.exe` from [GitHub Releases](https://github.com/RolinShmily/sekiro-mod-manager/releases).
The installer automatically sets up:
- **Sekiro Mod Manager Desktop** (`smm-desktop.exe`) with the authentic Sekiro icon.
- **SMM CLI** (`smm.exe`) in the application directory for terminal and AI-agent automation.

### 2. Build from Source
Ensure prerequisites are installed:
- [Rust](https://www.rust-lang.org/) (1.80+)
- [Node.js](https://nodejs.org/) (v20+) & [pnpm](https://pnpm.io/) (v9+)

```bash
# Clone the repository
git clone https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager

# Install frontend dependencies
pnpm install

# Run all workspace unit & integration tests
cargo test --workspace

# Run desktop GUI in development mode
pnpm run desktop:dev

# Build the one-click Windows installer (setup.exe)
pnpm run package
```

The resulting installer will be generated at `dist-installer/setup.exe` along with `dist-installer/SHA256SUMS.txt`.

---

## CLI Reference for AI-Agents & Power Users

The bundled CLI tool (`smm.exe`) provides full programmatic control over mod management:

```bash
# Inspect game directory health & ModEngine hook status
smm doctor --game-dir "D:\SteamLibrary\steamapps\common\Sekiro"

# List all mods in staging with priority and status
smm list --staging-dir "staging"

# Import a downloaded mod archive or directory with source attribution
smm import "D:\Downloads\WeaponMod.zip" --staging-dir "staging"

# Adjust deployment priority (lower number wins)
smm priority "kusabimaru-reaper" 5 --staging-dir "staging"

# Scan for path collisions and critical gameparam conflicts
smm scan --staging-dir "staging"

# Deploy all enabled mods via instant NTFS hard links
smm deploy --game-dir "D:\SteamLibrary\steamapps\common\Sekiro" --staging-dir "staging"

# Restore game mods directory to 100% pristine clean state
smm restore --game-dir "D:\SteamLibrary\steamapps\common\Sekiro"
```

---

## Sources & Attributions (Test Fixtures)

This project integrates representative community mods in `fixtures/mods/` strictly for algorithmic normalization and test suite validation. All intellectual property remains with their respective creators:

| Project Name | Author / Contributor | Source Repository | License | Role in Test Suite |
| :--- | :--- | :--- | :--- | :--- |
| **Sekiro Mod Engine** (v0.1.16) | **katalash** | [GitHub](https://github.com/katalash/ModEngine) / [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) | **GPL-3.0-or-later** | Base Loader & Hook environment checks (`dinput8.dll`, `modengine.ini`). |
| **Sekiro: Dream of the Damned** | **Nuffly** | [GitHub](https://github.com/nuffly/DotD) / [NexusMods #793](https://www.nexusmods.com/sekiro/mods/793) | **Apache-2.0** | Comprehensive overhaul validating `chr/`, `event/`, `map/`, and `gameparam` collision detection. |
| **Native PS4 Buttons** | **katalash** | [NexusMods #7](https://www.nexusmods.com/sekiro/mods/7) | **Custom Permissive** | UI & Scaleform resources (`menu/`, `font/`). |
| **Kusabimaru Reaper Weapon & Arm** | **Eyedea** | [NexusMods #350](https://www.nexusmods.com/sekiro/mods/350) | **CC-BY-NC-4.0** | Weapon and prosthetic slot mutual exclusivity (`wp_a_0300`, `am_m_9000`). |

*Note: The test fixtures contain stripped lightweight placeholder binaries and metadata strictly for topology and path testing, without commercial distribution of copyrighted full assets.*

---

## License

This project is licensed under the [MIT License](LICENSE).
Sekiro: Shadows Die Twice is a registered trademark of FromSoftware, Inc. and Activision. This project is an unofficial community tool.
