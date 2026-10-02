# Licensing & Third-Party Notices

Sekiro Mod Manager (SMM) is free and open-source software released under the [MIT License](LICENSE).
This document describes the scope of the license, third-party dependencies, and intellectual property notices.

---

## 1. Scope of the MIT License

The MIT License covers all original first-party source code in this repository:

| Component | Path | Description |
| :--- | :--- | :--- |
| **Core Engine** | `src/core/` | C++17 library (`smm_core`): normalization, conflict analysis, NTFS hard-link deployment, preset and modpack management |
| **Headless CLI** | `src/cli/` | Command-line tool (`smm.exe`) & JSON API for scripting and headless operation |
| **Desktop GUI** | `src/gui/` | Qt 6 Quick / QML native desktop application (`smm_gui.exe`) with HuskarUI components |
| **Build Configuration** | `CMakeLists.txt`, `CMakePresets.json` | Cross-platform CMake build system and compiler configurations |
| **Documentation** | `README.md`, `README.zh-CN.md`, `LICENSING.md` | User and developer guides, technical specifications |

---

## 2. Third-Party Components & Vendored Libraries

SMM links against and incorporates several open-source libraries. Each is governed by its own upstream license:

| Component | Upstream / Author | License | Usage in SMM |
| :--- | :--- | :--- | :--- |
| **HuskarUI** | [mengps/HuskarUI](https://github.com/mengps/HuskarUI) | **MIT** | Modern Ant Design style QML UI component library, theme tokens, and icon fonts (`third_party/HuskarUI`) |
| **nlohmann/json** | [nlohmann/json](https://github.com/nlohmann/json) | **MIT** | Header-only JSON library for metadata parsing and JSON serialization (`third_party/nlohmann/`) |
| **miniz** | [richgel999/miniz](https://github.com/richgel999/miniz) | **MIT** | Lossless data compression and ZIP archive reader/writer for mod import and `.smmpack` export (`third_party/miniz/`) |
| **Qt 6** | [The Qt Company](https://www.qt.io/) | **LGPLv3** / GPLv3 | Dynamic C++ / QML application framework. SMM dynamically links Qt 6 shared libraries and does not modify Qt itself. |

Detailed third-party copyright texts, vendor notices, and build flags are provided in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

---

## 3. Typography & Embedded Fonts

SMM embeds and bundles open-source fonts licensed under the [SIL Open Font License 1.1 (OFL-1.1)](http://scripts.sil.org/OFL):

| Font | Role in SMM | Copyright & Attribution | License |
| :--- | :--- | :--- | :--- |
| **Inter** | Latin body & interface prose text | Copyright (c) 2016-2023 The Inter Project Authors | **OFL-1.1** |
| **JetBrains Mono** | Digits, metrics, code blocks & UI telemetry chrome | Copyright (c) 2020 The JetBrains Mono Authors | **OFL-1.1** |
| **Instrument Serif** | Brand-only display serif headings | Copyright (c) 2022 The Instrument Serif Project Authors | **OFL-1.1** |
| **Noto Sans SC** | Chinese typography fallback stack (via system / DirectWrite) | Copyright (c) 2014-2025 Adobe, Google, and the Noto Project Authors | **OFL-1.1** |

---

## 4. Sekiro Mod Engine (ModEngine) is NOT Bundled

Sekiro Mod Engine (`dinput8.dll`) created by **katalash** is proprietary third-party software with **no published open-source license**:
- Its repository contains no `LICENSE` file.
- The readme states *"All rights reserved"* and permits redistribution only of an *unmodified* copy shipped *with a mod*.
- SMM is a mod manager, not a mod; therefore, redistributing ModEngine binaries is not permitted.

**Policy:**
SMM **never embeds, bundles, or redistributes** any `dinput8.dll` binary. Instead, SMM provides automated detection, diagnostics (`doctor`), and configuration patching for user-supplied ModEngine installations. Users should obtain ModEngine directly from verified community sources such as [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) or [github.com/katalash/ModEngine](https://github.com/katalash/ModEngine).

---

## 5. Community Mods & User Content

- Any mod packages installed, staged, imported, or deployed via SMM remain the intellectual property of their respective creators under their original licenses (e.g. Creative Commons, NexusMods Custom, Permissive).
- SMM acts solely as a local file management and linking utility; it does not relicense, claim ownership of, or sublicense user mod content.

---

## 6. Trademarks & Disclaimer

*Sekiro: Shadows Die Twice* is a registered trademark of FromSoftware, Inc. and Activision.
Sekiro Mod Manager (SMM) is an independent, unofficial community tool created by fans and contributors. It is not affiliated with, endorsed by, or sponsored by FromSoftware, Inc., Activision, or any of their affiliates.
