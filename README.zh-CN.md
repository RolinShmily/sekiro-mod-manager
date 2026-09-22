<div align="center">

# Sekiro Mod Manager (只狼模组管理器)

**专为《只狼：影逝二度》（Sekiro: Shadows Die Twice）量身定制的高性能、零磁盘开销专用桌面模组管理器与自动化调度核心**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Rust](https://img.shields.io/badge/Rust-1.80%2B-orange.svg)](https://www.rust-lang.org/)
[![Tauri](https://img.shields.io/badge/Tauri-v2-24C8D8.svg)](https://tauri.app/)
[![React](https://img.shields.io/badge/React-18-61DAFB.svg)](https://react.dev/)
[![Tailwind CSS](https://img.shields.io/badge/Tailwind-v3-38B2AC.svg)](https://tailwindcss.com/)

[English](README.md) | [简体中文](README.zh-CN.md)

</div>

---

## 核心特性

- ⚡ **Win32 NTFS 零开销秒级部署**：利用 Windows 原生 `CreateHardLinkW` 硬链接技术直接将 Mod 资产投射至游戏 `mods/` 目录，耗时毫秒级且**不额外占用一分一毫磁盘空间**。
- 🛡️ **纯净还原与防炸档机制**：每次部署均由 `.smm_manifest.json` 全息追踪。一键「还原纯净」仅定向摘除 SMM 建立的链接，绝对不碰游戏本体与未托管文件。
- 🔍 **智能目录归一化算法**：自动穿透并剥离压缩包内任意多层嵌套父文件夹，启发式将散落在根目录的文件（如 `c0000.chrbnd.dcx`）精准归位至只狼官方标准子路径（`chr/`, `parts/` 等）。
- ⚠️ **三级语义冲突裁决引擎**：
  - `[CRITICAL]` 核心数值冲突：精准拦截多模组对核心参数表 `gameparam.parambnd.dcx` 的物理覆盖冲突，预防坏档；
  - `[EXCLUSIVE SLOT]` 独占槽位互斥：识别主角体模（`c0000`）与武器（`wp_a_0300`）槽位，高优先级模组胜出，低优先级自动遮蔽；
  - `[INFO]` 普通覆盖：贴图、UI、音效按优先级正常接管。
- 📦 **来源全景溯源与整合包分发**：记录 Nexus Mods / GitHub / 网盘下载原址，支持附带原始压缩包导出单模组，以及一键打包/解包 `.smmpack` 模组整合包。
- 🤖 **AI-Agent 与自动化支持**：桌面端安装器同步将无头命令行工具（`smm.exe`）一并安装，方便 AI Agent 或终端脚本对模组进行全自动化管理。
- 🎨 **极简日式硬件级视觉体系**：严格落地 DESIGN.md 设计规范，纯白底色、经典战国朱砂金纹「隻狼」印玺图标、100% 纯矢量 SVG、首选 `"Maple Mono NF CN"` 等宽字体。

---

## 系统目录架构

```text
sekiro-mods/
├── Cargo.toml                         # Workspace 根配置
├── LICENSE                            # MIT 开源许可证
├── THIRD-PARTY-LICENSES.txt           # OFL 1.1 与 RARLAB UnRAR 许可证原文
├── README.md                          # 英文架构与使用文档
├── README.zh-CN.md                    # 中文架构与使用文档
├── DESIGN.md                          # 界面视觉与交互规范设计字典
├── package.json                       # 自动化脚本：构建、打包、测试
├── pnpm-workspace.yaml                # Monorepo 多包管理配置
├── crates/
│   ├── smm-core/                      # 只狼模组管理器核心算法与调度库
│   │   ├── src/
│   │   │   ├── conflict.rs            # 语义冲突检测与矩阵遮蔽引擎
│   │   │   ├── deploy.rs              # 胜出文件部署规划器 (DeploymentPlanner)
│   │   │   ├── doctor.rs              # ModEngine 环境自检与一键装配引擎
│   │   │   ├── executor.rs            # Win32 NTFS 硬链接物理执行与安全回滚
│   │   │   ├── exporter.rs            # 整合包 (.smmpack) 与单模组导出引擎
│   │   │   ├── extractor.rs           # 多格式原生解压引擎 (7z/zip/rar)
│   │   │   ├── importer.rs            # 智能归一化解包导入与来源溯源
│   │   │   ├── loader.rs              # 模组目录发现与元数据解析
│   │   │   ├── manager.rs             # 启停状态、优先级与元数据持久化
│   │   │   ├── normalizer.rs          # 启发式目录归一化算法与特征库
│   │   │   └── types.rs               # 强类型数据合约模型
│   │   └── tests/                     # 完整单元与集成测试套件
│   └── smm-cli/                       # 独立命令行工具 (`smm`)
│       └── src/main.rs                # 终端入口与完整命令实现
├── apps/
│   └── smm-desktop/                   # Tauri v2 + React 18 + Tailwind CSS 桌面 GUI
│       ├── src/                       # React 高品质组件库 (HeaderBar, ModList, ModDetails 等)
│       └── src-tauri/                 # Tauri v2 原生绑定与 IPC 桥接
├── dist-installer/                    # 发布产物输出目录 (Sekiro-Mod-Manager.exe, Sekiro-Mod-Manager-Setup.exe, smm-cli.exe)
└── scripts/
    └── build-installer.ps1            # 自动化一键打包流水线脚本
```

---

## 安装与发布产物

可直接从 [GitHub Releases](https://github.com/RolinShmily/sekiro-mod-manager/releases) 下载：
- **`Sekiro-Mod-Manager.exe`**：免安装绿色独立程序（双击直接启动 GUI，无需安装向导）；
- **`Sekiro-Mod-Manager-Setup.exe`**：Windows 标准安装包（提供安装向导并创建桌面及开始菜单图标）；
- **`smm-cli.exe`**：独立命令行终端工具（供 AI-Agent 自动化与脚本直接调用）。

### 从源码编译构建
确保本地已安装环境：
- [Rust](https://www.rust-lang.org/) (1.80+)
- [Node.js](https://nodejs.org/) (v20+) 与 [pnpm](https://pnpm.io/) (v9+)

```bash
# 克隆仓库
git clone https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager

# 安装前端依赖
pnpm install

# 运行全工作区单元与集成测试
cargo test --workspace

# 启动桌面 GUI 开发热重载模式
pnpm run desktop:dev

# 一键执行全自动打包流水线
pnpm run package
```

打包完成后，最终产物统一生成在 `dist-installer/` 目录，并附带 SHA-256 校验文件 `dist-installer/SHA256SUMS.txt`。

---

## 命令行 CLI 使用指南（供 AI-Agent 与终端自动化）

随安装包捆绑的 `smm.exe` 提供全量可编程接口：

```bash
# 检查游戏环境与 ModEngine 注入状态
smm doctor --game-dir "D:\SteamLibrary\steamapps\common\Sekiro"

# 查看暂存库内全部模组的优先级与启停状态
smm list --staging-dir "staging"

# 导入外部压缩包并自动归一化纳管
smm import "D:\Downloads\WeaponMod.zip" --staging-dir "staging"

# 调整模组部署优先级 (数值越小优先级越高，P:1 压制 P:10)
smm priority "kusabimaru-reaper" 5 --staging-dir "staging"

# 执行语义冲突与文件碰撞扫描
smm scan --staging-dir "staging"

# 一键毫秒级执行 NTFS 硬链接部署到游戏
smm deploy --game-dir "D:\SteamLibrary\steamapps\common\Sekiro" --staging-dir "staging"

# 还原游戏 mods 目录为 100% 初始纯净状态
smm restore --game-dir "D:\SteamLibrary\steamapps\common\Sekiro"
```

---

## 开源协议与第三方声明 (License & Third-Party Notices)

本项目采用 [MIT 开源许可证](LICENSE)。

### MIT 授权范围

MIT 许可**仅**覆盖本项目原创源代码：

| 覆盖部分 | 路径 |
| :--- | :--- |
| 核心引擎（归一化、冲突分析、硬链接部署、导入导出） | `crates/smm-core/` |
| 无头 CLI（`smm`） | `crates/smm-cli/` |
| React 18 + Tailwind 前端 | `apps/smm-desktop/src/` |
| Tauri v2 原生绑定与 IPC 层 | `apps/smm-desktop/src-tauri/` |
| 构建、打包、字体子集化脚本与 CI | `scripts/`、`.github/workflows/` |
| 项目文档 | `README.md`、`README.zh-CN.md`、`DESIGN.md` |

下列内容**不在**授权范围内，不授予任何权利：

- **第三方组件** —— 见下方[第三方许可证](#第三方许可证)。
- **Sekiro Mod Engine (ModEngine)** —— 完全未被再分发，见[下方说明](#modengine-未被捆绑)。
- **社区模组** —— `staging/` 目录及您运行时导入的模组，版权与许可归其各自作者所有（如
  `CC-BY-NC-4.0`、`Custom Permissive`）。SMM 仅为管理工具，不对模组内容进行再授权。测试套件中出现
  的社区模组名称（*Dream of the Damned*、*Native PS4 Buttons*、*Kusabimaru Reaper* 等）仅作为
  真实感的元数据使用：全部夹具均于运行时在临时目录（`tempfile::tempdir_in`）动态合成，
  **没有任何**第三方模组内容被再分发。
- **游戏资源与商标** —— 《只狼：影逝二度》及其全部内容均为 FromSoftware, Inc. 与 Activision 之商标
  与版权。本项目不包含任何游戏本体资源，与二者无隶属、背书或赞助关系。
- **品牌美术资源** —— `apps/smm-desktop/public/sekiro-logo.svg` 中的「隻狼」篆刻元素涉及游戏商标，
  仅用于标识本非官方社区工具。
- **发布二进制** —— `dist-installer/` 中的产物静态链接了下方第三方组件，故其分发除 MIT 外还同时受
  相应 upstream 许可证约束。

### ModEngine 未被捆绑

Sekiro Mod Engine（`dinput8.dll`，作者 **katalash**）属于无任何公开许可证的第三方专有软件：仓库内无
`LICENSE` 文件，GitHub API 返回 `license: null`（`/license` 端点 HTTP 404），而随 DLL 提供的 readme
声明 "All rights reserved"，仅允许再分发**未经修改**、且**与某个 mod 捆绑**用于启用该 mod 的副本。
SMM 是 mod 管理器而非 mod，因此不适用该授权。SMM **不内嵌、不捆绑、不随附、不代部署**任何 ModEngine
二进制，仅做检测与 `modengine.ini` 的生成/修正。

请自行从 [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) 或
[github.com/katalash/ModEngine](https://github.com/katalash/ModEngine) 下载 ModEngine，并提供其
`dinput8.dll`（参见上方 CLI 指南中的 `smm setup-engine`）。

### 第三方许可证

有两个组件以二进制形式再分发，其**许可证全文随每一次发布一并提供**，位于
[`THIRD-PARTY-LICENSES.txt`](THIRD-PARTY-LICENSES.txt)：

| 组件 | 许可证 |
| :--- | :--- |
| Inter、JetBrains Mono、Noto Sans SC —— 内嵌于应用的子集化 `.woff2` 字体 | SIL Open Font License 1.1 |
| RARLAB UnRAR —— 经 `unrar_sys 0.5.8` 引入，静态链接进 `smm.exe` 与 `Sekiro-Mod-Manager.exe` | UnRAR freeware license（非 OSI） |

> **UnRAR 限制：** 该许可证第 2 条禁止用其代码开发 RAR (WinRAR) 兼容压缩器。SMM 仅将其用于
> **只读 RAR 解压**；只要该依赖存在，就绝不能实现 RAR 压缩功能——需要打包请用 `.zip`。

直接运行时依赖（16 个 Rust crate、6 个 npm 包）如下。完整 upstream 文本可从
[crates.io](https://crates.io) 与 [npmjs.com](https://www.npmjs.com) 获取；`Cargo.lock` 与
`pnpm-lock.yaml` 中锁定的传递依赖各自沿用其原始许可。仅参与构建的工具链
（`vite`、`typescript`、`tailwindcss`、`postcss`、`autoprefixer`、`subset-font`）不产生任何发布产物。

| 生态 | 依赖 | 许可证 |
| :--- | :--- | :--- |
| Rust | `serde`、`serde_json`、`tempfile`、`thiserror`、`clap`、`windows-sys`、`unrar`（仅 wrapper）、`rfd`、`open`、`comfy-table` | MIT OR Apache-2.0 |
| Rust | `walkdir` | Unlicense OR MIT |
| Rust | `zip` | MIT |
| Rust | `sevenz-rust` | Apache-2.0 |
| Rust | `tauri`、`tauri-build` | Apache-2.0 OR MIT |
| Rust | `colored` | **MPL-2.0** —— file-level copyleft；自 crates.io 未经修改地使用，未修改也未分发任何受 MPL 覆盖的文件 |
| 前端 | `@tauri-apps/api` | Apache-2.0 OR MIT |
| 前端 | `react`、`react-dom`、`clsx`、`tailwind-merge` | MIT |
| 前端 | `lucide-react` | ISC |

<details>
<summary><code>lucide-react</code> 要求随附的 ISC 声明</summary>

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

`THIRD-PARTY-LICENSES.txt` 中的两段文本均为逐字原文。升级 `@fontsource/*` 或 `unrar_sys` 时请重新
核对：UnRAR 段对应 Cargo registry 中的 `unrar_sys-<版本>/vendor/unrar/license.txt`，OFL 正文来自
`node_modules/@fontsource/inter/LICENSE`。该文件缺失时打包流水线会直接中止。

《只狼：影逝二度》（Sekiro: Shadows Die Twice）系 FromSoftware, Inc. 与 Activision 之注册商标，
本项目为社区非官方开源工具。
