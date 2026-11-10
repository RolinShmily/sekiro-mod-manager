# Third-party notices / 第三方声明

SMM's own source code and documentation are MIT licensed, © 2025–2026 Sekiro Mod Manager
Contributors — see the root [LICENSE](LICENSE). That license does not replace the third-party
licenses below and grants no rights to FromSoftware, Activision or Sekiro trademarks and artwork.

SMM 自有源码与文档采用 MIT，作者为 Sekiro Mod Manager Contributors，全文见根目录 [LICENSE](LICENSE)。
根许可证不替代下列第三方许可证，也不授予 FromSoftware、Activision 或《只狼》商标与美术素材的任何权利。

Verbatim upstream texts live in [`licenses/`](licenses/). They are copied byte-for-byte and must not
be rewritten; `.gitattributes` pins them to LF so the shipped copies match upstream.

---

## 1. Scope of the MIT license / MIT 覆盖范围

The MIT license covers all original first-party code in this repository:

| Component | Path | Description |
| --- | --- | --- |
| **Core engine** | `src/core/` | C++17 library (`smm_core`): normalization, conflict analysis, NTFS hard-link deployment, preset and modpack management |
| **Headless CLI** | `src/cli/` | `smm.exe` and its machine-readable `--json` API |
| **Desktop GUI** | `src/gui/` | Qt 6 Quick / QML application (`smm_gui.exe`) built on HuskarUI |
| **Website** | `website/` | Static site and agent-skill download page (dependencies retain their own terms, see §3) |
| **Build configuration** | `CMakeLists.txt`, `CMakePresets.json`, `package.json`, `pnpm-workspace.yaml` | CMake and pnpm workspace configuration |
| **Documentation** | `README.md`, `README.zh-CN.md`, `docs/`, `THIRD_PARTY_NOTICES.md` | User and developer guides, technical specifications |

---

## 2. Desktop components / 桌面组件

| Component | Terms and copyright | License text |
| --- | --- | --- |
| **Qt 6 runtime** | LGPL-3.0 (or GPL-3.0, or commercial), The Qt Company and contributors; dynamically linked | [Qt notice, GPLv3 and LGPLv3](licenses/Qt-LGPL-3.0.txt) |
| **HuskarUI** | MIT, © 2026 mengps | [MIT](licenses/HuskarUI-MIT.txt) |
| **QWindowKit 1.5.1** | Apache-2.0, Stdware Collections and wangwenx190 | [Apache-2.0](licenses/QWindowKit-Apache-2.0.txt) |
| **StdCoreLib** | MIT, © 2022–present Stdware Collections | [MIT](licenses/StdCoreLib.txt) |
| **QR Code Generator** | MIT, © Project Nayuki | [MIT](licenses/QRCodeGenerator-MIT.txt) |
| **nlohmann/json 3.11.3** | MIT, © 2013–2023 Niels Lohmann | [MIT](licenses/nlohmann-json-MIT.txt) |
| **miniz 3.0.2** | MIT, © 2013–2014 RAD Game Tools and Valve Software; © 2010–2014 Rich Geldreich and Tenacious Software LLC | [MIT](licenses/miniz-MIT.txt) |

`nlohmann/json` and `miniz` are vendored under `src/core/third_party/`; their upstream `LICENSE`
files remain next to the sources and are mirrored here. HuskarUI is a pinned submodule, and
QWindowKit, StdCoreLib and the QR Code generator are built from within it. HuskarUI's icon font is
retained from that submodule; upstream documents no separate font license, and this repository does
not assert one.

Qt DLLs remain replaceable by compatible builds. Reverse engineering to debug modifications to
LGPL components is permitted under the LGPL. Matching Qt source and module-specific third-party
notices are available from [Qt source archives](https://download.qt.io/archive/qt/) and
[Qt third-party licensing](https://doc.qt.io/qt-6/licenses-used-in-qt.html). Anything that ships
`smm_gui` must keep Qt as replaceable shared libraries and reproduce the LGPL notice and the Qt
source offer.

Qt 动态库可替换为兼容版本；为调试 LGPL 组件而修改并逆向工程，属于 LGPL 授予的权利。发布
`smm_gui` 时必须保持 Qt 为可替换的动态库，并随附 LGPL 声明与 Qt 源码获取方式。

---

## 3. Website dependencies / 网站依赖

The website bundles production dependencies with different terms. The direct dependencies are:

| Component | Terms |
| --- | --- |
| React, React DOM | MIT |
| Radix UI (`@radix-ui/react-accordion`, `@radix-ui/react-tabs`) | MIT |
| Motion | MIT |
| Lucide (`lucide-react`) | ISC |
| Tailwind CSS, Vite, TypeScript (build time) | MIT / MIT / Apache-2.0 |

Do not treat the entire transitive dependency graph as MIT. `pnpm licenses:collect` reads the
installed production graph and writes `website/public/third-party-licenses.txt`; the website build
runs it, and the deployed site serves the generated file. Packages without a standalone license
file retain their declared terms and an upstream reference in that report.

网站打包的生产依赖各自保留许可条款，不能把整棵依赖树当作 MIT。`pnpm licenses:collect` 会读取已安装的
生产依赖图并生成 `website/public/third-party-licenses.txt`，网站构建会自动执行，部署后可直接访问。

---

## 4. Not bundled, on purpose / 有意不打包

### ModEngine (`dinput8.dll`)

SMM detects, configures and installs ModEngine, but **never ships it**. katalash's ModEngine
publishes no license and reserves all rights, so redistributing the DLL would not be lawful; the
only redistribution its author permits is an unmodified copy shipped with a mod. `doctor` and
`setup-engine` therefore require a genuine `dinput8.dll` (over 10 KB) to already exist in the
ModEngine source folder or in `staging/mod-engine/`, and report where to obtain it
(<https://www.nexusmods.com/sekiro/mods/6>) when it does not.

SMM 不内嵌、不打包、不再分发任何 `dinput8.dll`。ModEngine 没有公开许可证且保留所有权利，唯一允许的
再分发形式是随 mod 附带未经修改的原版副本。

### 7-Zip / WinRAR

`7z.exe` and `unrar.exe` are invoked as external processes when the user asks to import a `.7z` or
`.rar` package. Nothing is linked against them and nothing is redistributed; `.zip` needs no
external tool.

---

## 5. Community mods and user content / 社区 mod 与用户内容

Any mod installed, staged, imported or deployed through SMM remains the intellectual property of
its creator under its original terms (Creative Commons, NexusMods Custom, permissive, or
otherwise). SMM is a local file-management and linking utility; it does not relicense, claim
ownership of, or sublicense user mod content.

---

## 6. Trademarks and disclaimer / 商标与免责声明

*Sekiro: Shadows Die Twice* is a registered trademark of FromSoftware, Inc. and Activision.
SMM is an independent, unofficial community tool created by fans and contributors. It is not
affiliated with, endorsed by, or sponsored by FromSoftware, Inc., Activision, or any of their
affiliates.

《只狼：影逝二度》(Sekiro: Shadows Die Twice) 是 FromSoftware, Inc. 与 Activision 的注册商标。
本项目为玩家社区自主开发的独立工具，与上述公司无任何隶属、背书或赞助关系。
