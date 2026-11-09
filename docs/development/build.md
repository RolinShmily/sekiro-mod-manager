# 构建

## 前置条件

| 工具 | 版本 | 用途 |
|---|---|---|
| Windows | 10 / 11（64 位） | 只有 Windows 提供 `CreateHardLinkW`、卷序列号与 Steam 库发现 |
| Visual Studio | 2022（MSVC v143） | C++17 编译器 |
| CMake | 3.20+（预设按 3.25+ 验证） | 构建系统 |
| Qt | 6.5+，MSVC 2022 64-bit | GUI（`Quick`、`Qml`、`QuickControls2`、`Network`、`LinguistTools`） |
| Ninja | 1.11+（可选） | 快构建；预设默认使用 |
| Python 3（可选） | 任意 3.x | 配置期自动执行 `scripts/patch_huskarui.py` |

只构建核心与 CLI 时不需要 Qt，也不需要 Python。

## 克隆

```bash
git clone --recurse-submodules https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager
```

已经克隆但没带子模块：

```bash
git submodule update --init --recursive
```

HuskarUI 带嵌套子模块（qwindowkit、qmsetup、stdcorelib、QR-Code-generator），漏掉 `--recursive` 会在配置阶段以 `HuskarUI is missing` 失败。

## 预设

| 预设 | 说明 |
|---|---|
| `msvc-x64-debug` | Ninja + MSVC 2022 + Qt 6.11.2，Debug |
| `msvc-x64-release` | 同上，Release（GUI 为 `WIN32_EXECUTABLE`，无控制台窗口） |
| `core-only` | `BUILD_GUI=OFF`，只构建 `smm_core` + `smm` |
| `vs2022-x64` | Visual Studio 17 2022 生成器，普通终端可用（Ninja 预设需要 `vcvarsall` 环境） |

```bash
cmake --preset msvc-x64-debug
cmake --build --preset build-debug

cmake --preset core-only
cmake --build --preset build-core

cmake --preset vs2022-x64
cmake --build --preset build-vs2022-release
```

所有预设把 Qt 的 `bin` 目录前置到 `PATH`（`"PATH": "<qt>/bin;$penv{PATH}"`）。CMakePresets 的环境宏是 `$env{VAR}` 与 `$penv{VAR}`，写成 `${penv{PATH}}` 会让整个预设文件失效。

## CMake 选项

| 选项 | 默认 | 作用 |
|---|---|---|
| `BUILD_CLI` | `ON` | 构建 `smm.exe`（别名目标 `smm-cli`） |
| `BUILD_GUI` | `ON` | 构建 `smm_gui.exe`，需要 Qt 与 HuskarUI 子模块 |
| `SMM_WERROR` | `OFF` | 自有代码警告即错误（`/WX` / `-Werror`） |
| `SMM_BUILD_TESTS` | `OFF` | 构建并注册 4 个回归测试套件 |

预设里没有打开 `SMM_BUILD_TESTS`，需要测试时手动加：

```bash
cmake --preset msvc-x64-debug -DSMM_BUILD_TESTS=ON
cmake --build --preset build-debug
ctest --test-dir build/msvc-x64-debug --output-on-failure
```

`enable_testing()` 必须在任何 `add_subdirectory()` 之前调用，因此该开关只在顶层生效；子目录里的 `add_test()` 在开关关闭时会被静默忽略。

## 配置期发生了什么

1. `find_package(Python3)` 成功且存在 `scripts/patch_huskarui.py` → 执行补丁脚本（`OUTPUT_QUIET`）。
2. `add_subdirectory(src/core)` → 核心静态库 + 两个第三方目标（`smm_json` 公开、`smm_miniz` 私有）。
3. `BUILD_CLI` → `src/cli`。
4. `BUILD_GUI` → `src/gui`：校验 HuskarUI 存在、`add_subdirectory` 引入、对供应商目标递归加 `/w`、注册 QML 模块 `SmmGui`、编译翻译、嵌入图片资源。
5. `SMM_BUILD_TESTS` → 注册核心测试；GUI 层测试在 `src/gui/CMakeLists.txt` 内注册。

## 常见失败

**`HuskarUI is missing. Fetch the submodule first`**
子模块没初始化。运行 `git submodule update --init --recursive`。

**Ninja 构建报找不到 `windows.h` / `filesystem`**
从普通终端直接跑 Ninja 预设，MSVC 环境没加载。先执行：

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
```

或改用 `vs2022-x64`（MSBuild 自己找工具链）。

**`git status` 显示 `src/gui/third_party/HuskarUI (modified content)`**
配置期的补丁脚本改了三处第三方文件，属于预期状态。不要提交子模块内的改动，也不要 `git checkout` 掉它——下次配置会重新打上。

**运行 `smm_gui.exe` 报缺少 Qt DLL**
预设只影响 `cmake`/`ctest` 子进程的 `PATH`。手动运行时：

```bash
PATH="/c/Qt/6.11.2/msvc2022_64/bin:$PATH" ./build/msvc-x64-debug/src/gui/smm_gui.exe
```

**中文界面字体发虚 / 字重不一致**
先看设置页的字体选择。默认优先 `Noto Sans SC`；没有安装时回退到 `Microsoft YaHei UI`。字体在 `GuiController::applyFontFamily()` 中统一应用。

## 增量与清理

- 构建目录在 `build/<presetName>`，已被 `.gitignore` 忽略。
- 改了 `CMakeLists.txt` 或 QML 文件列表后增量构建会自动重新配置；改了 `qt_add_qml_module` 的 `QML_FILES` 若未生效，删掉 `build/<preset>/src/gui` 下的 `*_qmlcache_*` 再构建。
- 完整的 `rm -rf build/` 只在工具链或 Qt 版本变化时才需要。
