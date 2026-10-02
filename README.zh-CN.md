# 只狼模组管理器 (Sekiro Mod Manager · SMM)

专为《只狼：影逝二度》设计的高性能模组暂存、语义冲突仲裁与 NTFS 零拷贝硬链接部署管理器。

[English](README.md) | [简体中文](README.zh-CN.md)

---

## 项目简介

**Sekiro Mod Manager (SMM)** 是一款专为《只狼：影逝二度》打造的现代化轻量级原生模组管理工具。项目基于 **现代 C++17**、**Qt 6 / QML** 以及 **HuskarUI** 组件库重构，采用干净整洁的沙盒暂存区设计，通过 **NTFS 物理硬链接** 技术直接将生效模组投射至游戏 `mods/` 目录中，实现瞬间部署、零额外磁盘占用与一键纯净还原。

---

## 核心特性

- **⚡ 秒级 NTFS 零拷贝硬链接部署**
  通过 Windows `CreateHardLinkW` 底层接口将暂存区模组直链至游戏目录。无论数十 GB 的高清材质包还是复杂模型，均能在毫秒内投射完毕，零额外消耗硬盘空间。跨盘符时自动智能回退为稳健拷贝模式并提示诊断信息。

- **🛡️ 一键纯净还原**
  每次部署均通过 `.smm_manifest.json` 严格追踪记录。点击还原时仅精准清除 SMM 所投射的文件和空目录，绝不误删游戏原本的原生资产。

- **🧩 智能层级规整与多层压缩包深度解析**
  自动适配 Nexus Mods 及社区各类五花八门的压缩包结构：
  - 递归穿透多达 6 层的包裹外壳，准确定位 FromSoftware 游戏资产根目录；
  - 启发式归位散装文件（如根目录下裸放的 `wp_a_0300.partsbnd.dcx` 自动归位至 `parts/`，`c0000.chrbnd.dcx` 归位至 `chr/`，`.gfx` 归位至 `menu/font/`）；
  - 智能清洗 `_yabber` / `_witchy` / `.xml` 等解包工具残留脚手架。

- **⚖️ 三级语义冲突仲裁矩阵**
  支持自定义优先级拖拽与下拉顺位调谐（数值越小顺位越高），智能提示冲突级别：
  - **严重 (Critical)**：核心数值平衡表冲突（`gameparam.parambnd.dcx`）；
  - **警告 (Warning)**：同名排他性角色或武器槽位重叠；
  - **提示 (Info)**：一般性资源覆盖。

- **📦 跨端预设整合包 (.smmpack)**
  支持将当前的模组组合、启用状态与优先级整体打包导出为标准的 `.smmpack` 绿色分发包（含中英双语介绍与元数据），随时随地一键导入复原。

- **🎛️ 资产文件级启停与沉浸式卡片底图**
  支持直接在模组详情抽屉中查看其包含的全部子资产并单独开关；支持为模组卡片自定义高斯模糊磨砂底图，打造极具艺术感的管理体验。

- **🩺 ModEngine 全景健康诊断仪**
  内置环境检测向导，一键全景排查 `sekiro.exe` 主程序有效性、`dinput8.dll` 钩子有效性、`modengine.ini` 配置完整度及 NTFS 同卷匹配度，并提供一键自动注入与修复。

- **🌐 中英双语动态切换**
  内置完整的本地化翻译支持，可在设置中无缝即时切换简体中文 (`zh-CN`) 与英文 (`en-US`)。

---

## 模块架构

```
                 +-------------------+
                 |    smm_core       |  <-- 纯 C++17 静态库
                 |  (核心业务引擎)   |      (目录归一化、冲突检测、硬链部署、整合包)
                 +---------+---------+
                           |
             +-------------+-------------+
             |                           |
             v                           v
   +-------------------+       +-------------------+
   |     smm_cli       |       |     smm_gui       |
   | (smm.exe / JSON)  |       |   (smm_gui.exe)   |
   | CLI 命令行工具    |       | Qt 6 / QML + HuskarUI |
   +-------------------+       +-------------------+
```

| 模块 | 产物目标 | 技术栈 | 职责边界 |
|---|---|---|---|
| `src/core` | `smm_core` | C++17 静态库 | 纯净底层引擎：文件规整、冲突矩阵分析、NTFS 链接处理、ZIP 压缩解包。 |
| `src/cli` | `smm.exe` | C++17 可执行文件 | 独立的终端命令行工具与 JSON 自动化接口，便于脚本驱动与 AI Agent 集成。 |
| `src/gui` | `smm_gui.exe` | Qt 6 Quick / QML | 硬件加速的高颜值桌面客户端，进程内直接链接 `smm_core`，无任何中间进程损耗。 |

---

## 源码编译

### 环境准备

- **Windows 10 / 11 (64 位)**
- **Visual Studio 2022**（安装 MSVC v143 工具集，支持 C++17）
- **CMake 3.25+**
- **Qt 6.7+**（包含 `Qt6::Quick`、`Qt6::Qml`、`Qt6::LinguistTools` 模块）
- **Ninja**（可选，推荐配置以获得极速构建体验）

### 克隆仓库（包含子模块）

```bash
git clone --recurse-submodules https://github.com/RoL1n-SrP/sekiro-mods.git
cd sekiro-mods
```

### 编译与构建 (Build Instructions)

#### 推荐方式 1：普通终端直接构建（开箱即用，无需配置环境）
利用 Visual Studio 2022 预设，CMake 会全自动寻址 Windows SDK 与 MSVC 运行库，可在普通 PowerShell、Git Bash 或 CMD 中直接运行：

```bash
# Debug 构建
cmake --preset vs2022-x64
cmake --build --preset build-vs2022-debug

# Release 构建（独立发布版）
cmake --preset vs2022-x64
cmake --build --preset build-vs2022-release
```

#### 推荐方式 2：VS Code 内部一键构建
在 VS Code 中安装 **CMake Tools** 扩展：
1. 按 `Ctrl+Shift+P` -> 输入 `CMake: Select Configure Preset` -> 选择 `msvc-x64-debug` 或 `msvc-x64-release`；
2. 按 `F7` 即可一键并行编译，或按 `F5` 启动 GUI 调试并享受 QML 毫秒级热重载。
*(VS Code 会自动在后台为 Ninja 注入 MSVC 编译环境变量)*

#### 进阶方式 3：终端极速 Ninja 构建（需激活 MSVC 开发人员环境）
> ⚠️ **注意**：Ninja 是极简构建调度器，不会主动探测 Windows SDK 头文件目录。在**外部独立终端**中直接使用 Ninja 预设前，必须先加载 MSVC 开发人员环境（否则会提示找不到 `<filesystem>` / `<windows.h>`）：

```powershell
# 1. 激活 MSVC x64 开发环境（路径视具体 VS / BuildTools 安装位置而定）
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# 2. 执行 Ninja 预设极速构建
cmake --preset msvc-x64-release
cmake --build --preset build-release
```
*（也可以直接从 Windows 开始菜单打开 **“x64 Native Tools Command Prompt for VS 2022”** 执行上述 cmake 命令）*

---

## CLI 命令速查

SMM 提供功能完备的命令行工具（`smm.exe`），所有子命令均原生支持 `--json` 输出：

```bash
# 环境诊断与自动配置
smm.exe doctor
smm.exe doctor --json
smm.exe setup-engine

# 模组管理
smm.exe list
smm.exe enable <MOD_ID>
smm.exe disable <MOD_ID>
smm.exe set-priority <MOD_ID> <RANK>
smm.exe conflicts

# 部署与还原
smm.exe deploy
smm.exe restore

# 整合包管理
smm.exe pack-export my-pack.smmpack
smm.exe pack-import my-pack.smmpack
```

---

## 开源协议与版权声明

- 本项目源码采用 [MIT 许可证](LICENSE) 开源发布。
- 第三方组件与开源许可信息详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) 与 [LICENSING.md](LICENSING.md)。
- 《只狼：影逝二度》(Sekiro: Shadows Die Twice) 是 FromSoftware, Inc. 与 Activision 的注册商标。本项目为玩家社区自主开发的开源工具，与 FromSoftware 或 Activision 无任何商业关联。
