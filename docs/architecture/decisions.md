# 设计取舍与遗留问题

记录"为什么这样，而不是那样"。实现细节不在这里，在代码注释与 [data-flows.md](data-flows.md)。

## 已做的决定

### GUI 直接链接核心库，不走 CLI 子进程

**选择**：`smm_gui` 链接 `smm_core`，通过 `CoreService` 同步调用。

**理由**：部署要进度、要取消、要结构化错误；子进程意味着序列化、管道生命周期、僵尸进程和平台差异。核心本身零 Qt 依赖，链接它没有额外代价。

**代价**：核心的 ABI 变化会直接要求 GUI 重编。可接受，因为二者在同一仓库、同一提交里构建。

### 两个入口共享磁盘契约，而不是共享进程

CLI 与 GUI 互为"另一个客户端"：同一份 `.smm_mod.json`、`.smm_presets.json`、`.smm_manifest.json`。任何一侧改动文件格式，另一侧必须在同一提交内跟上。这也是 [../reference/file-formats.md](../reference/file-formats.md) 存在的原因。

### 优先级：数字小者胜，并列时 id 升序

**理由**：完全确定性。无论扫描顺序、目录枚举顺序、文件系统如何返回，同一个暂存区永远生成同一份计划。

**代价**：用户看到"顺位 1"才是最高优先，而元数据里存的是 `10`、`20` 这类数字。界面负责把数字解释成顺位（`ModCard.qml` 的 `modRank`），核心只认数字。

### 元数据只写 `.smm_mod.json`，`mod.json` 只读兼容

**选择**：`save_mod_info()` 只原子写 `.smm_mod.json`（临时文件 + `rename`，失败才回退直写），并删除同目录下遗留的 `mod.json`。

**理由**：早期版本双写两份，结果是外部工具改了 `mod.json`、SMM 改了 `.smm_mod.json`，谁的字段生效取决于读取顺序——同一份模组出现两套互相矛盾的元数据。现在只有一个权威写入点：`.smm_mod.json` 带 `disabled_assets`、`preview_image` 等扩展字段，读取时仍兼容只有 `mod.json` 的老包（读后立即迁移并删除旧文件）。

**代价**：仍用 `mod.json` 的生态工具改不动 SMM 管理的模组；它们写下的 `mod.json` 在下一次保存时会被隐式删除。

### 硬链接优先，复制兜底

**理由**：NTFS 同卷硬链接让几十 GB 的贴图包"部署"在毫秒级完成且不占额外空间。跨卷（暂存区在 C:、游戏在 D:）时硬链接非法，只能复制。

**代价**：复制路径会真实占用磁盘，且速度取决于数据量。因此 `staging` 体检项会写明当前是同卷还是跨卷，`DeployResult` 分别统计 `hard_links_created` 与 `copied_files`，让用户看见自己处在哪种模式，而不是以为"零拷贝"永远成立。

### 部署清单驱动还原，外加一次清扫

**理由**：只删清单里的文件会留下"中断的部署"和"手工拷进来的残留"；只清扫目录会删掉用户自己的东西。两者结合：先按清单精确删除，再清理空目录与残留条目，`RestoreResult` 分别报告 `removed_files` 与 `removed_dirs`。

### SMM 不打包、不伪造 ModEngine

**理由**：上游未声明可再分发许可，SMM 是管理器而不是模组，无法依赖"随模组附带"的授权。`provision_mod_engine()` 只在用户提供 payload 时安装；找不到就抛 `ModEngineSourceMissing`，并在 `MOD_ENGINE_SOURCE_HINT` 里给出获取途径。体检里 `MOD_ENGINE_DLL_MIN_SIZE` 用来识别占位用的假 DLL。

### GUI 声明 `requireAdministrator`，而不是让用户先关 Steam

**选择**：`smm_gui` 的 Windows 清单用 `requireAdministrator`（`QT_WINDOWS_APP_PROJECT_EXECUTION_LEVEL`），每次启动由系统先弹 UAC，再创建窗口。

**理由**：ModEngine 通过游戏目录里的 `dinput8.dll` 加载。非提权的启动链（SMM → Steam → 只狼）会被 Steam 的 `PreferSystem32` DLL 搜索策略抢走系统 `DINPUT8.dll`，所有模组静默失效；以前的规避办法是让用户先完全退出 Steam 再开管理器。提权从源头消掉这条失败模式（实测管理员权限下启动正常），顺带满足"往 `Program Files` 下的 Steam 库写文件需要写权限"这个本就存在的要求。

**代价**：没有管理员权限就无法运行 GUI（对模组管理器是合理要求）；同时 core 里的进程与 DLL 搜索策略体检（原 `runtime_doctor.cpp`）整体删除——问题不再可能出现，留着只会制造噪音。

### CLI 与 GUI 一致，同样声明 `requireAdministrator`

**选择**：`smm` 目标用链接器开关 `/MANIFESTUAC:level='requireAdministrator' uiAccess='false'`（`src/cli/CMakeLists.txt`），不用独立的 `.manifest` 文件。

**理由**：部署、还原、写 `modengine.ini` 都发生在游戏目录（通常是 `Program Files` 下的 Steam 库），非提权时这些写操作只得到 `Access is denied`；GUI 已在启动时统一提权，两个入口的权限语义一致，用户不必记"哪条命令要管理员"。之所以用 `/MANIFESTUAC` 而不是 `/MANIFESTINPUT` 挂一个清单文件：link.exe 自己生成的清单里已经有 `requestedExecutionLevel level="asInvoker"`，再合并一份就会出现两个同名元素，链接直接失败（`c1010001: Values of attribute "level" not equal in different manifest snippets`，报为 `LNK1327`）。

**代价**（不知道这条会误判成程序坏了）：`CreateProcess` 无法弹 UAC。cmd、PowerShell、VS Code 任务、Python `subprocess` 直接启动 `smm.exe` 一律得到 `ERROR_ELEVATION_REQUIRED`（740），**没有任何弹窗**；只有 shell 启动路径（资源管理器、`start smm ...`、`ShellExecuteEx` + `runas`）或已提权的父进程能拉起它。因此 `smm ... --json > out.json` 这类管道只能在提权终端里用，安装包把 CLI 加进用户 `PATH` 的承诺也只在提权终端成立；`scripts/local/*.ps1` 与 `skills/smm-cli` 都按此前提运行。CI 不受影响，因为 runner 进程本身是提权的。

### GUI 不做动效，也不再提供“减少动效”开关

**选择**：`main.cpp` 在创建 QML 引擎前固定 `HusTheme::instance()->setAnimationEnabled(false)`；QML 里不再出现 `Behavior` / `*Animation` / `Transition`；`GuiController` 删除 `reducedMotion` 属性、QSettings 键与设置页开关。

**理由**：换肤本身就会让整棵场景的颜色绑定在同一事件里变化，用 `Behavior on color` 过渡它只能得到十几帧的全窗重绘（以前的绕法是在 `applyThemeMode()` 里临时关掉再恢复，现在连绕的对象都没了）。这个界面的主要操作是长任务，进度与结果由进度条、状态标签与 Toast 文案承担，位移动画不增加信息；少一层“用户偏好 × 系统设置”也少一类验收状态。

**代价**：切页与悬停是硬切，没有过渡缓冲。对 1280×840 的桌面窗口可接受。HuskarUI 的 `durationFast/Mid` token 保留但当前不会被触发。规则写在 [../design/motion.md](../design/motion.md)。

### 设置页改成两列布局，而不是一张卡片里堆控件

**选择**：每个设置项是“标签 + 说明在左、控件在右”的 `RowLayout`，右列统一 `controlWidth = 340`；页内分组间距 30 > 组内项间距 18 > 标签与说明 4。路径类控件下面放带文字的按钮（浏览 / 打开 / 自动探测），不再用三个只有 tooltip 的图标按钮。

**理由**：上一版把标签、说明、输入框和三个图标按钮塞进同一行，密度由“能塞下多少”决定，说明文字被 `elide` 成一句话；改成两列后说明拿到整行宽度自然换行，控件对齐到同一条竖线，视线只需上下移动。图标按钮换成文字按钮是因为三个相同大小的图标需要悬停才能分辨。

**代价**：页面变长，1280×840 下字体预览需要滚动才能看全；右列宽度被固定后，超长路径在输入框内横向滚动。

### 体检只有五项，只在三个入口运行
**选择**：`diagnose_environment()` 把 ini 的三个键、目录与同卷状态、模组读取失败都折叠进各自的条目，总共五项；`Diagnose` 刷新只在打开启动器、点击「启动只狼」、诊断页点「检测环境」时发生，其余写操作只做 `Scan`，路径变更直接丢弃过期结果（`invalidateHealth()`）。

**理由**：条目越细，用户越难判断"下一步该动什么"；轮询式体检让空闲窗口持续扫盘。启动门禁仍然存在，只是从"按钮灰着"变成"点了就检，检出问题就拦下并跳到诊断页"。

### HuskarUI 作为 submodule，并用脚本打补丁

**理由**：UI 库需要跟版本走，submodule 固定提交可复现；上游对 Qt 6.8+ 的兼容问题用 `scripts/patch_huskarui.py` 在配置期自动修补，避免每个开发者手工改第三方代码。

**代价**：`git status` 永远显示 `src/gui/third_party/HuskarUI (modified content)`。这是预期状态，不是脏工作区。

### 后台任务串行，而不是并发

**理由**：暂存区与游戏 `mods/` 是共享可变资源。两个导入同时改名会互相覆盖，部署与还原并发会交错写清单。见 [async-threading.md](async-threading.md)。

### 警告策略按目标划分

`add_compile_options()` 是目录作用域，会被之后加入的供应商代码继承；结果是第三方警告淹没自有警告。改为 `smm::warnings` 接口目标 + 供应商目标 `/w`。见 [layers.md](layers.md)。

## 被否掉的方案

| 方案 | 否决原因 |
|---|---|
| FluentUI（`3rdparty/FluentUI`） | 需要打补丁的 Qt5Compat 依赖、15 MB 插件，且与 Qt 自带控件风格重复；已整体移除 |
| Kirigami | Windows 上需要 `qqc2-desktop-style` + `breeze-icons` 才正常渲染，实际是 KDE 视觉语言，与目标平台不符 |
| 用 CLI 子进程驱动 GUI | 见上：进度、取消、错误结构都要重新序列化 |
| 在 QML 里直接读文件系统 | 路径编码、异常处理与可测试性都会散落进视图 |
| 全量重建 `.smm_presets.json` 时静默丢弃损坏文件 | 用户存档不可再生，宁可报错 |

## 遗留问题

1. **核心诊断文案仍是英文。** `doctor.cpp` 生成的 `title` / `detail` 没有走 `tr()`（core 不链接 Qt）。GUI 按五个稳定 `id` 覆盖成中文（`GuiController::localizedTitle/Detail/Remediation`），CLI 保持英文；更彻底的做法是让 core 直接输出消息键。
2. **外部工具写 `mod.json` 不会生效，还会被删除。** 只有当 `.smm_mod.json` 缺失时才读 `mod.json`，读完立刻迁移；已有 `.smm_mod.json` 的模组，任何 `mod.json` 改动都被忽略，并在下次保存时清掉。
3. **`.github/PULL_REQUEST_TEMPLATE.md` 仍在引用 `cargo` / `pnpm` 命令**，与当前 C++/CMake 流程不符，需要替换为 `cmake --build` 与 `ctest`。
4. **`.editorconfig` 的默认缩进是 2 空格**，而现有 C++/QML 实际使用 4 空格；新增文件请与同目录现有文件保持一致，不要被默认值带偏。
5. **CLI 与 GUI 的设置不共享。** CLI 用 `%APPDATA%/SMM/settings.json`（可用 `SMM_SETTINGS_FILE` 覆盖），GUI 用 QSettings。两者各自解析路径，可能出现"CLI 认为暂存区在 A、GUI 认为在 B"。合并设置需要一次迁移设计。
6. **预览图缓存是静态的。** 热重载与删除时会清理，但外部直接改文件不会触发失效。
