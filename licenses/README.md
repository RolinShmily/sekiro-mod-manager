# Third-Party Licenses & Notices

This directory contains the licenses of third-party components that Sekiro Mod Manager (SMM)
**redistributes** (not merely depends on), plus an aggregated attribution list for its direct
runtime dependencies.

本目录收录 SMM 实际**再分发**（而非仅依赖）的第三方组件原始许可证，以及直接运行时依赖的
归属声明汇总。

---

## Contents / 目录内容

| File | Component covered | Upstream license | Why it must ship / 必须随附的原因 |
| :--- | :--- | :--- | :--- |
| [`OFL-1.1.txt`](OFL-1.1.txt) | Inter, JetBrains Mono, Noto Sans SC | SIL Open Font License 1.1 | Subsetted `.woff2` files are committed to `apps/smm-desktop/public/fonts/` and embedded in every release binary. The OFL requires the license text to accompany any redistribution of the font software. |
| [`UnRAR.txt`](UnRAR.txt) | RARLAB UnRAR (Alexander L. Roshal) | UnRAR freeware license (non-OSI, restrictive) | `unrar_sys` vendors the UnRAR C++ sources and compiles them into `smm-core`, so RARLAB code is statically linked into `smm.exe` and `Sekiro-Mod-Manager.exe`. Clause 2 mandates verbatim inclusion of its terms, and forbids using this code to build a RAR *compressor*. |
| [`THIRD-PARTY-NOTICES.md`](THIRD-PARTY-NOTICES.md) | Direct runtime dependencies (Rust + npm) | MIT / Apache-2.0 / ISC / MPL-2.0 / Unlicense | Statically linked or bundled into distributable artifacts; requires attribution. |
| [`LICENSE`](../LICENSE) | SMM's own license **and** scope declaration | MIT | One file: the verbatim MIT text followed by the NOTICE section stating exactly which parts of this repository the grant covers. |

> **There is no separate `NOTICE` file.** The scope declaration is appended to the root
> [`LICENSE`](../LICENSE) under a `NOTICE — SCOPE OF THE MIT LICENSE` heading, so the MIT text and the
> scope statement always travel together as one file. Any reference to "`NOTICE` §2.2" in this
> directory means that section of `LICENSE`.
>
> **本目录没有单独的 `NOTICE` 文件。** 范围声明附在仓库根目录 [`LICENSE`](../LICENSE) 的 MIT 正文
> 下方，以 `NOTICE — SCOPE OF THE MIT LICENSE` 为标题，使协议正文与范围声明始终作为一个文件分发。
> 本目录中引用的「`NOTICE` 第 2.2 节」均指 `LICENSE` 中的该节。

### Deliberately absent: Sekiro Mod Engine / 刻意未收录：Sekiro Mod Engine

There is intentionally **no** ModEngine license file here, because ModEngine is not redistributed in
any form. Upstream (`katalash/ModEngine`) publishes **no license at all** — the repository has no
LICENSE file and the GitHub API reports `license: null` (`/license` → HTTP 404) — while its bundled
readme declares "All rights reserved" and permits redistribution only of an *unmodified* copy
shipped *with a mod*. As a mod manager, SMM does not qualify for that grant, so it never embeds,
bundles or deploys `dinput8.dll`; users supply ModEngine themselves. See [§2 of the
notices](THIRD-PARTY-NOTICES.md) and [`../LICENSE`](../LICENSE) §2.2.

此处**刻意没有** ModEngine 的许可证文件，因为 ModEngine 未被以任何形式再分发。详见
[`LICENSE`](../LICENSE) 中 NOTICE 部分的第 2.2 节。

---

## Scope / 覆盖边界

**Included / 已覆盖**

- Components whose code or assets are physically redistributed by this repository or its
  release artifacts (fonts, vendored UnRAR sources).
- Direct runtime dependencies of the three workspace crates and the desktop frontend.

**Not included / 未覆盖**（and why / 原因）

- **Transitive dependencies** (~390 crates in `Cargo.lock`, hundreds of npm packages). These are
  consumed from their registries under their own licenses; see the lockfiles for the exact set.
  / 传递依赖数量庞大，各自沿用其注册表协议的原始许可，精确清单见 `Cargo.lock` 与 `pnpm-lock.yaml`。
- **Build-only tooling** (`vite`, `typescript`, `tailwindcss`, `postcss`, `autoprefixer`,
  `subset-font`, `@vitejs/plugin-react`). Never shipped inside release artifacts.
  / 仅参与构建，不进入发布产物。
- **Community mod assets** (`staging/`, imported mods). Each mod carries its own license; SMM is a
  management tool and does not relicense them. / 社区模组沿用各自协议，SMM 不对其进行再授权。
- **Sekiro Mod Engine / ModEngine.** Not redistributed at all — upstream publishes no license. See
  the "Deliberately absent" section above. / ModEngine 完全未被再分发，上游未公开任何许可证。
- **Game assets and trademarks.** *Sekiro: Shadows Die Twice* is a trademark of FromSoftware, Inc.
  and Activision. No game content is licensed by this project.

---

## Shipping / 分发接入

These files are not documentation-only — they are wired into the release pipeline:

| Where | Mechanism | Result |
| :--- | :--- | :--- |
| **NSIS installer** | `bundle.licenseFile` + `bundle.resources` in `apps/smm-desktop/src-tauri/tauri.conf.json` | Installs `<install dir>\licenses\` containing `LICENSE` and every file in this directory, next to the bundled `smm.exe`. |
| **Portable / CLI artifacts** | `scripts/build-installer.ps1` copies `LICENSE` and `licenses/*` into `dist-installer/` | The single-file `Sekiro-Mod-Manager.exe` and `smm-cli.exe` are published alongside these files, and `release.yml` attaches them to every GitHub Release. |
| **Build guard** | Step `[0/5] Compliance guard` in `scripts/build-installer.ps1` | The packaging pipeline **aborts before compiling** if any required license file is missing, so a release cannot ship without them. |

Because `tauri.conf.json` maps the directory with the glob `licenses/**/*` (flat destination), **any new
file added to this directory is picked up automatically** by the installer — no config change needed.
If you add a file that must also pass the guard, add it to `$requiredLicenseFiles` in
`scripts/build-installer.ps1`.

这些文件不只用于文档，而是已接入发布链路：NSIS 安装包经 `bundle.resources` 将整个 `licenses/`
安装到安装目录下；便携版与 CLI 由 `build-installer.ps1` 复制到 `dist-installer/` 并随 GitHub Release 发布；
打包脚本的 `[0/5]` 守卫会在任何许可证文件缺失时**在编译前直接中止**。由于使用 `licenses/**/*` 通配映射，
本目录新增文件会被安装包自动收录。

---

## Regenerating / 重新生成

`OFL-1.1.txt` and `UnRAR.txt` are extracted verbatim from their upstream sources so that they can
be re-verified against the pinned versions in `pnpm-lock.yaml` / `Cargo.lock`:

| File | Verbatim source |
| :--- | :--- |
| `OFL-1.1.txt` | license body from `node_modules/@fontsource/inter/LICENSE`; copyright lines from `@fontsource/inter`, `@fontsource/jetbrains-mono`, `@fontsource/noto-sans-sc` |
| `UnRAR.txt` | `<cargo-registry>/unrar_sys-0.5.8/vendor/unrar/license.txt` (pin: `unrar_sys 0.5.8`) |

When bumping `unrar_sys`, `@fontsource/*`, or any direct dependency, re-check this directory —
the copyright lines and versions recorded in `THIRD-PARTY-NOTICES.md` must be updated to match.
