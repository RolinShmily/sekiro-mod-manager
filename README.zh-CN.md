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
├── LICENSE                            # MIT 正文 + NOTICE 授权范围声明
├── licenses/                          # 第三方许可证全文与归属声明
│   ├── OFL-1.1.txt                    #   SIL OFL 1.1（Inter / JetBrains Mono / Noto Sans SC）
│   ├── UnRAR.txt                      #   RARLAB UnRAR 许可证（经 `unrar_sys` 引入）
│   └── THIRD-PARTY-NOTICES.md         #   直接运行时依赖归属清单
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

## 引用的第三方项目 (Referenced Third-Party Software)

SMM 仅在**名称与元数据层面**与下列社区项目发生关联。**本列表中的任何内容都未被 SMM 再分发**：
测试套件在运行时于临时目录（`tempfile::tempdir_in`）动态合成全部夹具，因此没有任何第三方 Mod 内容
——尤其是任何 ModEngine 二进制——被包含在本仓库或任何发布产物中。全部知识产权归各原作者所有。

| 项目 | 作者 | 来源 | Upstream 协议 | SMM 与其交互的方式 |
| :--- | :--- | :--- | :--- | :--- |
| **Sekiro Mod Engine** (v0.1.16) | **katalash** | [GitHub](https://github.com/katalash/ModEngine) / [NexusMods #6](https://www.nexusmods.com/sekiro/mods/6) | **未公开任何许可证**——专有，"all rights reserved" | SMM 仅检测已安装的 `dinput8.dll` 并生成/修正 `modengine.ini`。**须由您自行下载 ModEngine**；SMM 从不内嵌、捆绑或代其部署二进制。 |
| **Sekiro: Dream of the Damned** | **Nuffly** | [GitHub](https://github.com/nuffly/DotD) / [NexusMods #793](https://www.nexusmods.com/sekiro/mods/793) | Apache-2.0 | 作为合成冲突检测夹具（`chr/`、`event/`、`map/`、`gameparam`）的命名与元数据参照。 |
| **Native PS4 Buttons** | **katalash** | [NexusMods #7](https://www.nexusmods.com/sekiro/mods/7) | Custom Permissive | 作为合成 UI 资源夹具（`menu/`、`font/`）的命名与元数据参照。 |
| **Kusabimaru Reaper Weapon & Arm** | **Eyedea** | [NexusMods #350](https://www.nexusmods.com/sekiro/mods/350) | CC-BY-NC-4.0 | 作为合成武器专属槽位夹具（`wp_a_0300`、`am_m_9000`）的命名与元数据参照。 |

*Upstream 协议仅供参照，不代表我们代原作者做出声明，请以原始出处为准。上表中 ModEngine 的许可状态已
经 GitHub API 核实（`repos/katalash/ModEngine/license` 返回 HTTP 404）。*

---

## 开源协议

本项目采用 [MIT 开源许可证](LICENSE)。

**MIT 授权范围：** 仅覆盖本项目原创源代码（`crates/`、`apps/`、`scripts/`），**不**涵盖第三方组件、
社区模组、游戏资源与商标。[LICENSE](LICENSE) 文件为 MIT 正文 + 其后的 `NOTICE — SCOPE OF THE MIT
LICENSE` 范围声明；upstream 许可证全文与归属声明见 [licenses/](licenses/) 目录：

- [licenses/OFL-1.1.txt](licenses/OFL-1.1.txt) — Inter、JetBrains Mono、Noto Sans SC（内嵌子集字体）
- [licenses/UnRAR.txt](licenses/UnRAR.txt) — RARLAB UnRAR（经 `unrar_sys` 静态链接）
- [licenses/THIRD-PARTY-NOTICES.md](licenses/THIRD-PARTY-NOTICES.md) — 直接运行时依赖

**ModEngine 未被捆绑。** Sekiro Mod Engine（`dinput8.dll`）是 katalash 的第三方软件，未公开任何许可证，
因此 SMM 不以任何形式再分发它。需由您自行提供；SMM 仅做检测并生成 `modengine.ini`。

《只狼：影逝二度》（Sekiro: Shadows Die Twice）系 FromSoftware, Inc. 与 Activision 之注册商标，
本项目为社区非官方开源工具。
