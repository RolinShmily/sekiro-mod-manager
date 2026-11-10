# 代码风格

没有 `.clang-format`，风格由现有文件决定：**新代码与同目录现有文件保持一致**比任何规则都重要。

## C++

- 标准：C++17。不用 C++20 特性（`std::format`、`span`、概念），因为 MSVC 目标是 v143 且项目显式设置 `CXX_STANDARD 17`。
- 缩进 4 空格，行宽约 100 列，指针/引用贴类型（`const fs::path& path`）。
- 命名：
  - 类型 `PascalCase`（`ModManager`、`DeployResult`）；
  - 函数与变量 `snake_case`（`build_plan`、`staging_dir`）；
  - 常量 `kPascalCase` 且放在匿名命名空间（`kExitUsage`、`kCommands`）；
  - Qt 类成员 `camelCase_`（`statusText_`、`tasks_`）。
- 命名空间：核心 `smm`，CLI `smm::cli`，GUI `smm::gui`。匿名命名空间用于文件内实现细节。
- 头文件：`#pragma once`，路径与目录对应（`smm/loader.hpp`）。公开头文件不包含 Qt、不包含 `<windows.h>`。
- 前向声明优先：能 `struct ScanOutcome;` 就不要 `#include`，`doctor.hpp` 就是这么做的。

### 错误处理

- 核心统一 `fail(ErrorCode::X, "消息", path)` 或 `throw SmmError(...)`。**不要**抛 `std::runtime_error`（测试里的 `check()` 除外）。
- 捕获具体异常并附加上下文；禁止 `catch (...) {}` 静默吞掉。
- 错误码是公开契约，见 [../reference/errors.md](../reference/errors.md)：只增不改名。
- 能恢复的失败要降级而不是中止：`ScanOutcome::failures` 记录读不了的模组，`DeployResult::errors` 记录单个文件失败，其余继续。

### 平台代码

Windows 专属 API 放在 `#ifdef _WIN32` 后面，并给出非 Windows 的降级分支（哪怕只是 `fail(Unsupported)`）。`executor.cpp`、`doctor.cpp` 是参考实现。

### 注释

- 注释解释**为什么**，不复述代码。`types.hpp` 里 `priority` 写的是"Lower number wins. 1 takes precedence over 10."，而不是"the priority field"。
- 核心与 CLI 的注释用英文；GUI backend 与 QML 里有大量中文注释（历史原因），跟随所在文件。
- 修 bug 时把触发条件写进注释：例如 `executor.cpp` 里"Anything still lying around was not tracked (an interrupted deploy, a hand-copied mod, a lost manifest)"。下一个维护者需要知道为什么这里要清扫。

## QML

- 每个文件以 `pragma ComponentBehavior: Bound` 开头（`Main.qml` 与全部组件/视图都是），避免隐式上下文注入带来的调试困难。
- 根对象 `id: root`；属性声明集中在顶部，按"入参 → 内部状态 → 只读派生"排列。
- 必填属性用 `required property`（`Repeater` 委托里的 `required property var model` 必须有，否则拿不到模型）。
- 颜色、圆角只用 `HusTheme.Primary.*`；深色/浅色差异用 `HusTheme.isDark` 三元表达式。
- 不写 `Behavior` / `*Animation` / `Transition`：GUI 无动效，状态变化直接换值（见 [../design/motion.md](../design/motion.md)）。
- 图标只用 `HusIcon.*`，不写 emoji，不写字形私有区字符。
- 布局：优先 `Layout.*` 附加属性；需要固定命中区时同时设置 `Layout.preferredHeight` 与 `Layout.minimumHeight`（见 `NavItem.qml`）。
- 事件处理不要写业务逻辑：发信号，交给视图或 `smmBackend`。
- 不认识的控制器属性不要猜：`smmBackend` 的可用属性以 `GuiController.hpp` 为准，拼错只会在运行时报 `undefined`。

## CMake

- 目标名小写下划线（`smm_core`、`smm_gui`），别名用 `smm::` 前缀或 `smm-cli` / `smm-gui` 这种连字符形式。
- 警告选项挂在 `smm::warnings` 上，不用 `add_compile_options()`（目录作用域会漏给供应商代码）。
- 供应商目标用 `/w` / `-w`，并在注释里说明为什么（见 `src/gui/CMakeLists.txt` 的 `smm_silence_vendor_warnings`）。
- 注释写清"为什么这样写"，特别是看起来多余的部分：`CMakePresets.json` 里关于 `$penv{PATH}` 的说明就是为了防止有人"顺手修正"成 `${penv{PATH}}`。

## 测试

- 测试是独立可执行文件，不引入测试框架；`check(condition, "name")` 失败即抛异常，进程退出码非零。
- 每个断言的名字要能被单独读懂（`"stale deployment blocks"`），因为 CI 只会打印它。

## 文档

- 中文正文，代码/路径/命令保持原样。
- 交叉引用用相对路径。
- 新增或修改 CLI 参数、JSON 字段、磁盘文件、QML 组件公开属性时，同步改对应分册——这是提交的一部分，不是后续任务。
