# Sekiro Mod Manager (SMM)

High-performance mod staging, semantic conflict arbitration, and zero-copy NTFS deployment for *Sekiro: Shadows Die Twice*.

[English](README.md) | [简体中文](README.zh-CN.md)

---

## Overview

**Sekiro Mod Manager (SMM)** is a modern, lightweight, native mod manager engineered specifically for *Sekiro: Shadows Die Twice*. Built in modern **C++17**, **Qt 6 / QML**, and **HuskarUI**, it keeps mods cleanly staged in isolated folders and projects active files directly into the game's `mods/` directory using **NTFS hard links** — achieving instantaneous zero-disk-space deployments with pristine rollback capability.

---

## Key Features

- **⚡ Zero-Copy NTFS Deployment**
  Enabled mods are projected into the game directory via Windows `CreateHardLinkW`. Deploying dozens of gigabytes of high-definition textures or character models takes milliseconds and consumes zero additional disk storage. SMM automatically detects cross-volume setups and gracefully falls back to copy mode with diagnostic feedback.

- **🛡️ Pristine Rollback**
  Every deployment state is tracked via `.smm_manifest.json`. Restoring purges only files and directories linked or copied by SMM, preserving the pristine state of your game installation.

- **🧩 Smart Normalization & Multi-depth Archive Parsing**
  Handles arbitrary folder hierarchies from Nexus Mods or community zip/7z packages. Recursively extracts nested archives up to 6 levels deep, routes loose signature files (`wp_a_0300.partsbnd.dcx` → `parts/`, `c0000.chrbnd.dcx` → `chr/`, `.gfx` → `menu/font/`), and cleans unpacking scaffolding (`_yabber`, `_witchy`).

- **⚖️ 3-Tier Semantic Conflict Arbiter**
  Automatic collision resolution by user-assigned priority rank:
  - **Critical**: Collisions on global balance tables (`gameparam.parambnd.dcx`).
  - **Warning**: Overlaps on exclusive player/weapon slots.
  - **Info**: General asset overrides.

- **📦 Mod Pack Presets (.smmpack)**
  Save, apply, export, and import entire curated mod configurations in portable `.smmpack` archives (with bilingual Chinese/English titles and descriptions).

- **🎛️ Fine-Grained Asset Toggles & Immersive Backdrops**
  Inspect individual asset files inside any staged mod and toggle them on or off individually. Customize mod cards with blurred immersive backdrop artwork.

- **🩺 Environment Health Doctor**
  Built-in diagnostic suite verifying `sekiro.exe`, ModEngine `dinput8.dll`, `modengine.ini` injection parameters, and NTFS volume consistency with one-click automated repair.

- **🌐 Bilingual Internationalization (i18n)**
  Seamless real-time switching between Simplified Chinese (`zh-CN`) and English (`en-US`).

---

## Architecture

```
                 +-------------------+
                 |    smm_core       |  <-- Pure C++17 static library
                 |  (Core Engine)    |      (Normalization, Conflicts, Deploy, Presets)
                 +---------+---------+
                           |
             +-------------+-------------+
             |                           |
             v                           v
   +-------------------+       +-------------------+
   |     smm_cli       |       |     smm_gui       |
   | (smm.exe / JSON)  |       |   (smm_gui.exe)   |
   | CLI & Agent API   |       | Qt 6 / QML + HuskarUI |
   +-------------------+       +-------------------+
```

| Component | Target | Tech Stack | Description |
|---|---|---|---|
| `src/core` | `smm_core` | C++17 static lib | Zero-overhead core logic: file normalization, conflict matrix, NTFS link engine, ZIP archive extractor/packager. |
| `src/cli` | `smm.exe` | C++17 executable | Standalone CLI and headless JSON API for automated workflows and scripting. |
| `src/gui` | `smm_gui.exe` | Qt 6 Quick / QML | Hardware-accelerated desktop GUI linking `smm_core` directly in-process for instant response times. |

---

## Building from Source

### Prerequisites

- **Windows 10 / 11 (64-bit)**
- **Visual Studio 2022** (MSVC v143 toolset with C++17 support)
- **CMake 3.25+**
- **Qt 6.7+** (with `Qt6::Quick`, `Qt6::Qml`, `Qt6::LinguistTools`)
- **Ninja** (optional, recommended for fast builds)

### Clone with Submodules

```bash
git clone --recurse-submodules https://github.com/RoL1n-SrP/sekiro-mods.git
cd sekiro-mods
```

### Build via CMake Presets (Ninja / MSVC)

```bash
# Debug Build with Live QML Hot-Reload Support
cmake --preset msvc-x64-debug
cmake --build --preset build-debug

# Release Build (Optimized standalone binary)
cmake --preset msvc-x64-release
cmake --build --preset build-release
```

### Build via Visual Studio Solution

```bash
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64"
cmake --build build --config Release --target smm_gui
```

---

## CLI Reference

SMM includes a headless CLI (`smm.exe`) for scripting, CI, or AI agent integration. Every command supports `--json` for structured output.

```bash
# General environment & diagnostics
smm.exe doctor
smm.exe doctor --json
smm.exe setup-engine

# Mod management
smm.exe list
smm.exe enable <MOD_ID>
smm.exe disable <MOD_ID>
smm.exe set-priority <MOD_ID> <RANK>
smm.exe conflicts

# Deployment
smm.exe deploy
smm.exe restore

# Mod packs (.smmpack)
smm.exe pack-export my-pack.smmpack
smm.exe pack-import my-pack.smmpack
```

---

## Licensing & Trademarks

- SMM source code is licensed under the [MIT License](LICENSE).
- Third-party dependencies and notices are detailed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [LICENSING.md](LICENSING.md).
- *Sekiro: Shadows Die Twice* is a registered trademark of FromSoftware, Inc. and Activision. SMM is an independent community project not affiliated with FromSoftware or Activision.
