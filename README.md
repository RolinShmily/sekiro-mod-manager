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
├── LICENSE                            # MIT text + NOTICE scope declaration
├── licenses/                          # Third-party license texts & attribution
│   ├── OFL-1.1.txt                    #   SIL OFL 1.1 (Inter / JetBrains Mono / Noto Sans SC)
│   ├── UnRAR.txt                      #   RARLAB UnRAR license (via `unrar_sys`)
│   └── THIRD-PARTY-NOTICES.md         #   Direct runtime dependency attribution
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
├── dist-installer/                    # Release artifacts (Sekiro-Mod-Manager.exe, Sekiro-Mod-Manager-Setup.exe, smm-cli.exe)
└── scripts/
    └── build-installer.ps1            # Automated one-click packaging pipeline
```

---

## Installation & Releases

Download the latest releases from [GitHub Releases](https://github.com/RolinShmily/sekiro-mod-manager/releases):
- **`Sekiro-Mod-Manager.exe`**: Portable standalone desktop app (run directly without installation wizard).
- **`Sekiro-Mod-Manager-Setup.exe`**: Windows setup installer (creates Start Menu & desktop shortcuts).
- **`smm-cli.exe`**: Standalone command-line interface for terminal and AI-agent automation.

### Build from Source
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

# Build the release artifacts
pnpm run package
```

The resulting artifacts will be generated in `dist-installer/` along with `dist-installer/SHA256SUMS.txt`.

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

## Referenced Third-Party Software

SMM interoperates with the following community projects **by name and metadata only**. Nothing in
this list is redistributed with SMM: the test suite synthesises all of its fixtures at runtime in
throwaway temp directories (`tempfile::tempdir_in`), so no third-party mod content — and in
particular no ModEngine binary — is included in this repository or in any release artifact. All
intellectual property remains with the respective creators.

| Project | Author | Source | Upstream license | How SMM interacts with it |
| :--- | :--- | :--- | :--- | :--- |
| **Sekiro Mod Engine** (v0.1.16) | **katalash** | [GitHub](https://github.com/katalash/ModEngine) / [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) | **None published** — proprietary, "all rights reserved" | SMM detects an installed `dinput8.dll` and generates/patches `modengine.ini`. **You must download ModEngine yourself**; SMM never embeds, bundles, or deploys its binary. |
| **Sekiro: Dream of the Damned** | **Nuffly** | [GitHub](https://github.com/nuffly/DotD) / [NexusMods #793](https://www.nexusmods.com/sekiro/mods/793) | Apache-2.0 | Used as a naming/metadata reference in synthetic conflict-resolution fixtures (`chr/`, `event/`, `map/`, `gameparam`). |
| **Native PS4 Buttons** | **katalash** | [NexusMods #7](https://www.nexusmods.com/sekiro/mods/7) | Custom Permissive | Used as a naming/metadata reference in synthetic UI-resource fixtures (`menu/`, `font/`). |
| **Kusabimaru Reaper Weapon & Arm** | **Eyedea** | [NexusMods #350](https://www.nexusmods.com/sekiro/mods/350) | CC-BY-NC-4.0 | Used as a naming/metadata reference in synthetic weapon-slot exclusivity fixtures (`wp_a_0300`, `am_m_9000`). |

*Upstream licenses are listed for reference only and are not asserted on the authors' behalf —
verify them at the source. The ModEngine license status above was confirmed via the GitHub API
(`repos/katalash/ModEngine/license` returns HTTP 404).*

---

## License

This project is licensed under the [MIT License](LICENSE).

**Scope of the MIT grant:** the MIT License covers only our own source code (`crates/`, `apps/`,
`scripts/`). It does **not** extend to third-party components, community mods, game assets, or
trademarks. The [LICENSE](LICENSE) file carries the MIT text followed by a `NOTICE — SCOPE OF THE
MIT LICENSE` section spelling that out, and the [licenses/](licenses/) directory holds the upstream
license texts and attribution:

- [licenses/OFL-1.1.txt](licenses/OFL-1.1.txt) — Inter, JetBrains Mono, Noto Sans SC (bundled subset fonts)
- [licenses/UnRAR.txt](licenses/UnRAR.txt) — RARLAB UnRAR (statically linked via `unrar_sys`)
- [licenses/THIRD-PARTY-NOTICES.md](licenses/THIRD-PARTY-NOTICES.md) — direct runtime dependencies

**ModEngine is not bundled.** Sekiro Mod Engine (`dinput8.dll`) is third-party software by katalash
with no published license, so SMM does not redistribute it in any form. You supply it yourself;
SMM only detects it and configures `modengine.ini`.

Sekiro: Shadows Die Twice is a registered trademark of FromSoftware, Inc. and Activision. This
project is an unofficial community tool.
