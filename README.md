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
├── licenses/                          # Verbatim third-party license texts
│   ├── OFL-1.1.txt                    #   SIL OFL 1.1 (Inter / JetBrains Mono / Noto Sans SC)
│   └── UnRAR.txt                      #   RARLAB UnRAR license (via `unrar_sys`)
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

## License & Third-Party Notices

This project is licensed under the [MIT License](LICENSE).

### Scope of the MIT grant

The MIT License covers **only our own source code**:

| Covered | Path |
| :--- | :--- |
| Core engine (normalization, conflicts, hard-link deployment, import/export) | `crates/smm-core/` |
| Headless CLI (`smm`) | `crates/smm-cli/` |
| React 18 + Tailwind frontend | `apps/smm-desktop/src/` |
| Tauri v2 native bindings and IPC layer | `apps/smm-desktop/src-tauri/` |
| Build, packaging and font-subsetting scripts, CI | `scripts/`, `.github/workflows/` |
| Documentation | `README.md`, `README.zh-CN.md`, `DESIGN.md` |

It does **not** extend to any of the following, and grants no rights to them:

- **Third-party components** — see [Third-party licenses](#third-party-licenses) below.
- **Sekiro Mod Engine (ModEngine)** — not redistributed at all; see [below](#modengine-is-not-bundled).
- **Community mods** — `staging/`, and anything you import at runtime, remains the property of its
  respective authors under its own license (e.g. `CC-BY-NC-4.0`, `Custom Permissive`). SMM is a
  management tool and does not relicense, sublicense, or grant any rights to mod content. Community
  mod names such as *Dream of the Damned*, *Native PS4 Buttons* and *Kusabimaru Reaper* appear in
  the test suite only as realistic metadata: every fixture is synthesised at runtime in a throwaway
  temp directory (`tempfile::tempdir_in`), so **no** third-party mod content is redistributed.
- **Game assets and trademarks** — *Sekiro: Shadows Die Twice* and everything in it are trademarks
  and copyright of FromSoftware, Inc. and Activision. SMM ships no game assets and is not
  affiliated with, endorsed by, or sponsored by them.
- **Branding artwork** — `apps/smm-desktop/public/sekiro-logo.svg` uses a 隻狼-inspired seal motif
  referencing the game's trademark, and is provided only to identify this unofficial community tool.
- **Release binaries** — artifacts in `dist-installer/` statically link the components below, so
  their distribution is governed by those upstream licenses in addition to the MIT License.

### ModEngine is not bundled

Sekiro Mod Engine (`dinput8.dll`) by **katalash** is proprietary third-party software with **no
published license**: its repository contains no `LICENSE` file and the GitHub API reports
`license: null` (`/license` returns HTTP 404), while the readme bundled with the DLL states
"All rights reserved" and permits redistribution only of an *unmodified* copy shipped *with a mod*.
SMM is a mod manager rather than a mod, so that grant does not cover it. SMM therefore never embeds,
bundles, ships, or deploys any ModEngine binary — it only detects an installed `dinput8.dll` and
generates/patches `modengine.ini`.

Download ModEngine yourself from [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) or
[github.com/katalash/ModEngine](https://github.com/katalash/ModEngine), then supply its `dinput8.dll`
(see `smm setup-engine` in the [CLI reference](#cli-reference-for-ai-agents--power-users)).

### Third-party licenses

Two components are redistributed in binary form, and their **full license texts ship with every
release** in [`licenses/`](licenses/):

| Component | License | Full text |
| :--- | :--- | :--- |
| Inter, JetBrains Mono, Noto Sans SC — subset `.woff2` fonts embedded in the app | SIL Open Font License 1.1 | [`licenses/OFL-1.1.txt`](licenses/OFL-1.1.txt) |
| RARLAB UnRAR — vendored via `unrar_sys 0.5.8`, statically linked into `smm.exe` and `Sekiro-Mod-Manager.exe` | UnRAR freeware license (non-OSI) | [`licenses/UnRAR.txt`](licenses/UnRAR.txt) |

> **UnRAR constraint:** clause 2 of that license forbids using the code to build a RAR (WinRAR)
> compatible archiver. SMM uses it for **read-only RAR extraction only**; RAR compression must never
> be implemented while this dependency is present — distribute `.zip` instead.

Direct runtime dependencies (16 Rust crates, 6 npm packages). Full upstream texts are available from
[crates.io](https://crates.io) and [npmjs.com](https://www.npmjs.com); transitive dependencies pinned
in `Cargo.lock` and `pnpm-lock.yaml` remain under their own licenses. Build-only tooling
(`vite`, `typescript`, `tailwindcss`, `postcss`, `autoprefixer`, `subset-font`) ships no artifact.

| Ecosystem | Dependencies | License |
| :--- | :--- | :--- |
| Rust | `serde`, `serde_json`, `tempfile`, `thiserror`, `clap`, `windows-sys`, `unrar` (wrapper only), `rfd`, `open`, `comfy-table` | MIT OR Apache-2.0 |
| Rust | `walkdir` | Unlicense OR MIT |
| Rust | `zip` | MIT |
| Rust | `sevenz-rust` | Apache-2.0 |
| Rust | `tauri`, `tauri-build` | Apache-2.0 OR MIT |
| Rust | `colored` | **MPL-2.0** — file-level copyleft; consumed unmodified from crates.io, no MPL-covered file is modified or distributed |
| Frontend | `@tauri-apps/api` | Apache-2.0 OR MIT |
| Frontend | `react`, `react-dom`, `clsx`, `tailwind-merge` | MIT |
| Frontend | `lucide-react` | ISC |

<details>
<summary>ISC notice required by <code>lucide-react</code></summary>

```text
ISC License

Copyright (c) for portions of Lucide are held by Cole Bemis 2013-2022 as part of
Feather (MIT). All other copyright (c) for Lucide are held by Lucide Contributors 2022.

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice and
this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```

</details>

The two bundled texts in `licenses/` are verbatim copies. When bumping `@fontsource/*` or `unrar_sys`,
re-check them: `licenses/UnRAR.txt` mirrors `unrar_sys-<version>/vendor/unrar/license.txt` in the Cargo
registry, and the OFL body in `licenses/OFL-1.1.txt` comes from `node_modules/@fontsource/inter/LICENSE`.
The packaging pipeline aborts if either file is missing.

*Upstream licenses for the community projects referenced above are listed for convenience only and
are not asserted on their authors' behalf — verify them at the source. ModEngine's license status
was confirmed via the GitHub API (`repos/katalash/ModEngine/license` → HTTP 404).*

Sekiro: Shadows Die Twice is a registered trademark of FromSoftware, Inc. and Activision. This
project is an unofficial community tool.
