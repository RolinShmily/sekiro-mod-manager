# Sekiro Mod Engine (v0.1.16)

**Author:** katalash  
**Repository:** https://github.com/katalash/ModEngine  
**NexusMods:** https://www.nexusmods.com/sekiro/mods/6  
**License:** GNU General Public License v3.0 (GPL-3.0)

---

## Overview

Sekiro Mod Engine is an injection library designed for *Sekiro: Shadows Die Twice* that intercepts file access calls to enable loading unpacked/loose files directly from a designated directory (default `\mods`). It removes the requirement to unpack multi-gigabyte game archives (`Data0.bdt` through `Data5.bdt`) via UXM, preserving clean game files and speeding up installation.

## Key Features
- **DLL Search-Order Hijack (`dinput8.dll`)**: Proxies original DirectInput8 calls while hooking game file I/O.
- **AOB Pattern Scan**: Dynamically locates the file loading routine across different Sekiro executable updates (v1.02 - v1.06).
- **Loose File Redirection**: Diverts file lookups to `modOverrideDirectory` (default: `\mods`).
- **Chain Loading Support**: Forwards non-hooked input calls to system DirectInput.

## Installation
1. Copy `dinput8.dll` and `modengine.ini` into your Sekiro main installation directory (where `sekiro.exe` is located).
2. Create a folder named `mods` in your Sekiro game directory.
3. Place mod files (e.g. `parts/`, `chr/`, `param/`) inside `mods/`.

## Configuration (`modengine.ini`)
- `enabled`: Set to 1 to enable mod loading, 0 to disable.
- `loadUXMFiles`: Set to 0 if using loose files in `mods/`.
- `modOverrideDirectory`: The directory containing loose mod assets (e.g. `\mods`).

## License
Mod Engine is licensed under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. See `LICENSE` for the full license text.
