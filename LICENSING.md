# Licensing & Third-Party Notices

SMM's own source code is released under the [MIT License](LICENSE). This document records exactly
what that grant covers, what it does **not** cover, and the verbatim upstream license texts that ship
with every release.

The upstream texts themselves live in [`licenses/`](licenses/) and are pinned to LF by
`.gitattributes` so that the published copies stay byte-identical to the originals.

---

## 1. Scope of the MIT grant

The MIT License covers **only our own source code**:

| Covered | Path |
| :--- | :--- |
| Core engine (normalization, conflicts, hard-link deployment, import/export) | `crates/smm-core/` |
| Headless CLI (`smm`) | `crates/smm-cli/` |
| React 18 + Tailwind frontend | `apps/smm-desktop/src/` |
| Tauri v2 native bindings and IPC layer | `apps/smm-desktop/src-tauri/` |
| Build, packaging and font-subsetting scripts, CI | `scripts/`, `.github/workflows/` |
| Documentation | `README.md`, `README.zh-CN.md`, `LICENSING.md` |

It does **not** extend to any of the following, and grants no rights to them:

- **Third-party components** — see [§3](#3-third-party-licenses).
- **Sekiro Mod Engine (ModEngine)** — not redistributed at all; see [§2](#2-modengine-is-not-bundled).
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

---

## 2. ModEngine is not bundled

Sekiro Mod Engine (`dinput8.dll`) by **katalash** is proprietary third-party software with **no
published license**: its repository contains no `LICENSE` file and the GitHub API reports
`license: null` (`/license` returns HTTP 404), while the readme bundled with the DLL states
"All rights reserved" and permits redistribution only of an *unmodified* copy shipped *with a mod*.
SMM is a mod manager rather than a mod, so that grant does not cover it. SMM therefore never embeds,
bundles, ships, or deploys any ModEngine binary — it only detects an installed `dinput8.dll` and
generates/patches `modengine.ini`.

Download ModEngine yourself from [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) or
[github.com/katalash/ModEngine](https://github.com/katalash/ModEngine), then supply its `dinput8.dll`
(see `smm setup-engine` in the [CLI reference](README.md#cli-reference-for-ai-agents--power-users)).

---

## 3. Third-party licenses

Two components are redistributed in binary form, so their **verbatim license texts ship with every
release** in [`licenses/`](licenses/):

| Component | License | Text |
| :--- | :--- | :--- |
| Inter, JetBrains Mono, Noto Sans SC — subset `.woff2` fonts embedded in the app | SIL Open Font License 1.1 | [`OFL-1.1.txt`](licenses/OFL-1.1.txt) |
| RARLAB UnRAR — vendored via `unrar_sys 0.5.8`, statically linked into `smm.exe` and `Sekiro-Mod-Manager.exe` | UnRAR freeware license (non-OSI) | [`UnRAR.txt`](licenses/UnRAR.txt) |

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

The files in `licenses/` are verbatim upstream texts. When bumping `@fontsource/*` or
`unrar_sys`, re-check them: the UnRAR section mirrors
`unrar_sys-<version>/vendor/unrar/license.txt` in the Cargo registry, and the OFL body comes from
`node_modules/@fontsource/inter/LICENSE`. The packaging pipeline aborts if either file is missing.

*Upstream licenses for the community projects referenced above are listed for convenience only and
are not asserted on their authors' behalf — verify them at the source. ModEngine's license status
was confirmed via the GitHub API (`repos/katalash/ModEngine/license` → HTTP 404).*

---

## 4. Trademarks

Sekiro: Shadows Die Twice is a registered trademark of FromSoftware, Inc. and Activision. This
project is an unofficial community tool and is not affiliated with, endorsed by, or sponsored by
them.
