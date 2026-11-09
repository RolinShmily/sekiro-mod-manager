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

**想用另一套库跑 GUI（演示、截图、多开对比）**
不要改设置页或注册表，直接把两个目录传给进程：

```bash
./build/msvc-x64-release/src/gui/smm_gui.exe --staging=D:/demo/staging --game-dir=D:/demo/game
```

`--staging=` / `--game-dir=` 只对本次进程生效，不写回 `QSettings`（`HKCU\Software\SekiroModManager\SMM`），
所以用户的保存设置不受影响。截图脚本 `scripts/local/capture_screenshots.ps1` 就靠这个参数指向演示库。
`--theme=dark|light|system` 同理，只覆盖本次运行的主题。

**中文界面字体发虚 / 字重不一致**
先看设置页的字体选择。默认优先 `Noto Sans SC`；没有安装时回退到 `Microsoft YaHei UI`。字体在 `GuiController::applyFontFamily()` 中统一应用。

## 增量与清理

- 构建目录在 `build/<presetName>`，已被 `.gitignore` 忽略。
- 改了 `CMakeLists.txt` 或 QML 文件列表后增量构建会自动重新配置；改了 `qt_add_qml_module` 的 `QML_FILES` 若未生效，删掉 `build/<preset>/src/gui` 下的 `*_qmlcache_*` 再构建。
- 完整的 `rm -rf build/` 只在工具链或 Qt 版本变化时才需要。

## 打包便携版与安装包

发版流程和本地验证用同一个脚本，避免“本机量的体积”和“用户下载的体积”不是一回事：

```powershell
$env:QT_ROOT_DIR = 'C:/Qt/6.11.2/msvc2022_64'
pwsh scripts/package-windows.ps1 -BuildDir build/msvc-x64-release -Version v0.4.5
```

输出在 `dist/`：GUI 便携目录 + GUI/CLI 两个 zip；检测到 `ISCC.exe` 时顺带生成 Inno Setup 安装包（`-SkipInstaller` 可跳过）。

脚本做的不只是 `windeployqt`，还有四件事，改动前请先读完理由：

1. **只保留 `QtQuick.Controls.Basic`**。`src/gui/main.cpp` 里 `QQuickStyle::setStyle("Basic")` 把这个选择钉死，否则 Windows 上默认样式（`FluentWinUI3`）会连带它的插件和 QML 一起进安装包。
2. **丢弃工具件**：`qmltooling`、`generic` 插件组和所有 `*.qmltypes` 只服务调试器和 Qt Creator，运行时从不读取。
3. **丢弃软件渲染回退与未用编译器**：`opengl32sw.dll`（约 20 MB）、`D3Dcompiler_47.dll`（Windows 10 1809+ 自带副本）、`dxcompiler.dll`（约 14 MB，实测默认与 `QSG_RHI_BACKEND=d3d12` 都不需要）。
4. **运行时用 DLL 而不是安装器**：`--compiler-runtime` 在能发现工具链时会塞进 25 MB 的 `vc_redist.x64.exe`；脚本删掉它，改为带 3 个 CRT DLL（约 2.5 MB），并同步进 CLI 包。

**`Qt6OpenGL.dll` 必须保留**：`Qt6Quick.dll` 硬导入它（`dumpbin /dependents Qt6Quick.dll` 可复现），D3D11 路径下同样需要；缺失时进程在 QML 加载前就以 127 退出——这正是旧发布流程的问题：它把 `Qt6OpenGL.dll` 当"未使用"删掉了。

实测（Qt 6.11.2 / Release，同一台机、同一套参数）：

| | 未裁剪 | 打包脚本 |
|---|---|---|
| GUI 目录 | 114.6 MB | **54.9 MB**（-52%） |
| GUI zip | 60.6 MB | **22.3 MB**（-63%） |

裁剪后的目录 **3/3 次启动成功**（`Get-Process` 存活 + 拿到窗口句柄），且带上了 VC++ 运行时，不依赖目标机预装 `vc_redist`。

脚本在压缩前会断言必需项（`Qt6OpenGL.dll`、`platforms/qwindows.dll`、Basic 样式、CRT…）仍然存在，裁剪写错时在打包阶段就失败，而不是等用户双击。

### 安装包装了什么

`installer/installer.iss` 的 `[Files]` 复制整个 GUI 目录，所以装完 `{app}` 里同时有：

- `SekiroModManager.exe`（GUI）、`smm.exe`（CLI）、Qt 运行时与 CRT；
- `skill/smm-cli.zip` 与解压好的 `skill/smm-cli/`（`SKILL.md`、`references/`、`scripts/`、`LICENSE.txt`）。压缩包由 `website/scripts/package_skill.py` 生成，和官网下载的是同一个文件，白名单保持一致。

「命令行」任务（默认勾选）会把 `{app}` 追加到**当前用户** `PATH`（`HKCU\Environment`）。安装与卸载两个方向都写在 `[Code]` 里，而不是用 `[Registry]` 的自动回滚：

- 安装时只在末尾追加，已存在则跳过（避免重复安装叠加条目）；
- 卸载时按 `;` 拆分、只丢掉 `{app}` 这一项，保留用户顺序与后来添加的其他项；
- `ChangesEnvironment=yes` 让安装结束后广播环境变更；已经打开的终端与资源管理器仍需重启才能看到新 `PATH`。

裁剪后务必跑一次冒烟，而且要看**进程是否存活**，不要只看截图：

```powershell
$p = Start-Process "dist/sekiro-mod-manager-<ver>-windows-x64-gui/SekiroModManager.exe" -PassThru
Start-Sleep 6
Get-Process -Id $p.Id -ErrorAction SilentlyContinue   # 拿不到就是缺 DLL，已退出
```

缺一个硬依赖 DLL 时进程会立刻退出（退出码 127），窗口根本不会出现；只截到白屏或黑图都说明不了问题。
