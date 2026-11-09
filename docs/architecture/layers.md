# 分层与边界

## 三个层，两个方向

```
src/core   ←  src/cli
           ←  src/gui
```

依赖只能朝箭头方向。反向依赖（core 里 `#include <QString>`、cli 里调用 GUI 类型）在编译期就会被抓住，因为目标是分离的：`smm_core` 不链接任何 Qt 目标。

## src/core：业务规则

| 关注点 | 头文件 | 说明 |
|---|---|---|
| 类型与序列化 | `smm/types.hpp` | `ModInfo`、`AssetEntry`、`ConflictReport`、`DeployPlan`、`ModPreset`、`StagedMod`，全部带 `to_json` / `from_json` |
| 错误 | `smm/error.hpp` | `ErrorCode` 是稳定契约，字符串形式进 JSON |
| 规范化 | `smm/normalizer.hpp` | 任意目录结构 → 游戏内相对路径 |
| 扫描 | `smm/loader.hpp` | 暂存区 → `ScanOutcome`（含失败列表） |
| 生命周期 | `smm/manager.hpp` | 启用、优先级、改名、删除、元数据写回 |
| 冲突 | `smm/conflict.hpp` | 按目标路径分组，低优先级数字胜出 |
| 计划与执行 | `smm/deploy.hpp`、`smm/executor.hpp` | 计划生成、硬链接/复制、清单、还原 |
| 归档 | `smm/extractor.hpp`、`smm/importer.hpp`、`smm/exporter.hpp` | zip（内置 miniz）、7z/rar（外部工具）、`.smmpack` |
| 预设 | `smm/preset.hpp` | `.smm_presets.json` 的读写与应用 |
| 环境 | `smm/doctor.hpp` | 体检、`modengine.ini` 修补、ModEngine 部署、进程提权状态 |

core 允许做的事：读写文件系统、抛 `SmmError`、解析 JSON。

core 禁止做的事：

- `std::cout` / `printf` / `std::cerr`。输出由 CLI 决定，GUI 走自己的通知。
- 链接 Qt。需要路径编码时用 `path_to_utf8()` / `utf8_to_path()`，不要引入 `QString`。
- 文案本地化。`DiagnosticItem::title` / `detail` 目前是英文，由界面负责翻译或直接展示。
- 长任务进度回调之外的状态。进度通过 `DeployProgressCallback` / `ExtractProgressCallback` 之类的参数注入，core 不保存回调。

## src/cli：机器与人的两种输出

- `cli_parser.cpp` 里的 `CommandSpec` 表是唯一事实来源：解析、`--help`、误用报错都从它生成，三者不会漂移。
- `commands/cmd_*.cpp` 每个命令返回一个 `nlohmann::json`，由 `json_output.cpp` 统一套信封。
- 人类可读输出（表格、颜色、进度条）只在非 `--json` 模式出现；`--json` 模式 stdout 上只有一行一个 JSON 对象。
- 退出码只有三个：`0` 成功、`1` 运行失败、`2` 用法错误（见 [../reference/cli.md](../reference/cli.md)）。

新增 CLI 命令的完整改动面：`cli_parser.cpp` 的 `kCommands`、`commands.hpp` 声明、`main.cpp` 分发、一个 `cmd_*.cpp` 实现、`src/cli/CMakeLists.txt` 源文件列表、`docs/reference/cli.md` 与 `json-contract.md`。

## src/gui：适配层

GUI 只做三件事：把核心结果映射成 Qt 类型、把用户意图翻译成核心调用、维护纯界面状态。细节见 [gui-backend.md](gui-backend.md)。

GUI 禁止做的事：

- 在 QML 里直接调用 `std::filesystem` 或拼接磁盘路径（`localPath()` 是唯一的 url → path 转接口）。
- 在模型里写盘。模型发出"请求"信号，控制器决定是否落到核心。
- 自己实现冲突裁决、优先级重排或规范化规则。

## 编译选项落在哪

`CMakeLists.txt` 定义了 `smm::warnings`（MSVC `/W4 /permissive-`，其他 `-Wall -Wextra -Wpedantic`），自有目标显式链接它。供应商代码（HuskarUI 及其嵌套子模块）由 `smm_silence_vendor_warnings()` 递归加 `/w`。

不要用 `add_compile_options()` 设置警告级别：它是目录作用域的，会被之后 `add_subdirectory()` 的供应商代码继承，最终结果是一堆第三方警告淹没自有代码的警告。

## 新代码放哪：判断顺序

1. 这条规则 CLI 和 GUI 都要用吗？要 → `src/core`。
2. 只是命令行参数解析或输出格式？→ `src/cli`。
3. 只是 Qt 类型转换、模型投影或界面状态？→ `src/gui/backend` 对应的角色目录。
4. 只是视觉与布局？→ `src/gui/qml`。

跨层复制的第一个副本就是下一次不一致的来源，宁可多写一次 `CoreService` 转发方法，也不要在 QML 里重算。
