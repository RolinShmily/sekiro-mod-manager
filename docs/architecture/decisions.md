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
