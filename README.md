<div align="center">

# Sekiro Mod Manager (SMM)

**High-Performance, Zero-Disk-Copy Desktop Mod Manager & Automation Core for *Sekiro: Shadows Die Twice***

[![CI](https://github.com/RolinShmily/sekiro-mod-manager/actions/workflows/ci.yml/badge.svg)](https://github.com/RolinShmily/sekiro-mod-manager/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/RolinShmily/sekiro-mod-manager?sort=semver)](https://github.com/RolinShmily/sekiro-mod-manager/releases)
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
- 🎨 **Minimalist Japanese Hardware Aesthetic**: Styled strictly according to modern design tokens, stark white canvas, classic "隻狼" cinnabar gold seal logo, 100% vector SVG icons, and self-hosted subset fonts (Inter, JetBrains Mono, Noto Sans SC) — no CDN and no network access at runtime.

---

## System Architecture

```text
sekiro-mods/
├── Cargo.toml                  # Rust workspace root (smm-core, smm-cli, smm-desktop)
├── package.json                # pnpm scripts: build, package, test
├── pnpm-workspace.yaml         # pnpm monorepo workspace
├── LICENSE                     # MIT License
├── LICENSING.md                # MIT scope & third-party notices
├── README.md                   # English documentation
├── README.zh-CN.md             # Simplified Chinese documentation
├── .github/workflows/          # CI (fmt + clippy + tests) and tagged-release pipelines
├── crates/
│   ├── smm-core/               # Core engine library
│   │   ├── src/
│   │   │   ├── conflict.rs     # Multi-level semantic conflict analyzer
│   │   │   ├── deploy.rs       # Winning-asset deployment planner
│   │   │   ├── doctor.rs       # ModEngine diagnostic & setup engine
│   │   │   ├── error.rs        # Typed error surface
│   │   │   ├── executor.rs     # Win32 NTFS hard-link executor & rollback
│   │   │   ├── exporter.rs     # Modpack (.smmpack) & single-mod export engine
│   │   │   ├── extractor.rs    # Multi-archive extractor (zip / 7z / rar)
│   │   │   ├── importer.rs     # Normalizing archive importer & provenance
│   │   │   ├── loader.rs       # Mod directory discovery & metadata parser
│   │   │   ├── manager.rs      # Lifecycle, priority & metadata persistence
│   │   │   ├── normalizer.rs   # Heuristic directory normalization algorithm
│   │   │   ├── preset.rs       # Activation preset engine
│   │   │   └── types.rs        # Strongly typed data contracts
│   │   └── tests/              # Automated integration test suites
│   └── smm-cli/                # Standalone CLI tool (`smm`)
│       └── src/
│           ├── cli.rs          # clap command & flag definitions
│           ├── commands/       # One module per subcommand
│           ├── output.rs       # Table & colour formatting
│           └── resolve.rs      # Staging / target path resolution
├── apps/smm-desktop/           # Tauri v2 + React 18 + Tailwind CSS desktop GUI
│   ├── src/                    # React app (api / components / utils)
│   └── src-tauri/              # Tauri v2 native bindings & IPC handlers
├── scripts/
│   ├── build-installer.ps1     # Automated one-click packaging pipeline
│   └── subset-fonts.mjs        # Offline font subsetting
└── licenses/                   # Verbatim upstream license texts
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
smm list --staging "staging"

# Import a downloaded mod archive or directory with source attribution
smm import "D:\Downloads\WeaponMod.zip" --staging "staging"

# Adjust deployment priority (lower number wins)
smm priority "kusabimaru-reaper" 5 --staging "staging"

# Scan for path collisions and critical gameparam conflicts
smm scan --staging "staging"

# Deploy all enabled mods via instant NTFS hard links into the game's mods/ directory
smm deploy --target "D:\SteamLibrary\steamapps\common\Sekiro\mods" --staging "staging"

# Restore the game's mods directory to a 100% pristine clean state
smm restore --target "D:\SteamLibrary\steamapps\common\Sekiro\mods"
```

---

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for the development setup, the
quality gates CI enforces, and the commit and pull-request conventions. Please follow the
[Code of Conduct](CODE_OF_CONDUCT.md), and report security issues privately as described in
[SECURITY.md](SECURITY.md).

## License & Credits

Released under the [MIT License](LICENSE). The MIT grant covers only SMM's own source code;
third-party components, the deliberate non-redistribution of ModEngine, and the required upstream
notices are documented in **[LICENSING.md](LICENSING.md)**, with the verbatim license texts shipped
in [`licenses/`](licenses/).

SMM is an unofficial community tool. *Sekiro: Shadows Die Twice* is a registered trademark of
FromSoftware, Inc. and Activision; this project is not affiliated with, endorsed by, or sponsored
by them.
