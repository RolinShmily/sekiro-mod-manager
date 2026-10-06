---
name: smm-cli
description: >-
  使用 Sekiro Mod Manager 的 smm.exe CLI 管理《只狼》模组：检查暂存区与游戏环境、导入压缩包、分析覆盖冲突、调整优先级和资产开关、部署与还原、管理预设、导入导出 .smmpack。用户提及 smm、smm-cli、只狼模组部署/冲突/整合包，或需要脚本化调用 SMM 时使用本技能，即使没有明确要求技能。纯 QML 外观、字体或 GUI 动效修改不属于本技能。
compatibility: Windows 上可执行的 smm.exe；直接调用不需要 Python。附带的 JSON 调用器需要 Python 3.9+ 标准库。.7z/.rar 解压可能需要已安装的 7-Zip 或 WinRAR。
---

# SMM CLI

通过 `smm.exe` 完成操作，使用真实命令输出确认结果。技能可独立复制使用；不要假定用户安装了源码、Qt 或特定构建工具。

## 1. 找到程序并确定路径

1. 优先使用用户给出的可执行文件，随后查找 PATH 上的 `smm`/`smm.exe`。在本仓库中可检查 `build/msvc-x64-release/src/cli/smm.exe`、Debug 或 VS2022 构建目录。
2. 执行 `smm --version`、`smm --help`，以当前程序为准核对命令。二进制可能落后于源码。
3. 使用 `smm --json env` 查看解析到的暂存区和游戏根目录；如果用户给了路径，为每次调用显式传 `--staging` 与 `--game-dir`。不要为了临时操作自动执行 `config`。
4. `--game-dir` 指向包含 `sekiro.exe` 的安装根目录；CLI 部署目标固定为其下的 `mods/`，不要传入 `mods/` 本身。
5. 当程序不存在时报告查找位置。在用户要求构建、且源码和工具链已就绪时，只构建 CMake 目标 `smm`；不要安装 Qt 来运行独立 CLI。

CLI 持久化设置与 GUI 的 QSettings 不同：CLI 默认使用 `%APPDATA%/SMM/settings.json`，可通过 `SMM_SETTINGS_FILE` 指向隔离配置。以 `env.data.settings_file` 和解析结果为准，不要假定 GUI 设置已同步。路径解析顺序是命令行参数、CLI 设置、自动探测。

## 2. 选择正确的命令

需要具体语法、结果字段和已知限制时，读取 [references/commands.md](references/commands.md)。常用命令：

| 意图 | 命令 |
| --- | --- |
| 查看路径 / 环境 | `env` / `doctor` |
| 列出模组 / 查看资产 | `list` / `info MOD_ID` |
| 检查覆盖 / 部署预览 | `conflicts` / `plan` |
| 导入普通压缩包或目录 | `import PATH`，先用 `--dry-run` 检查 |
| 调整覆盖顺位 | `priority MOD_ID NUMBER`，数值越小越优先 |
| 启用 / 禁用模组 | `enable MOD_ID` / `disable MOD_ID` |
| 启用 / 禁用文件 | `enable-asset MOD_ID REL_PATH` / `disable-asset MOD_ID REL_PATH` |
| 部署 / 清理部署目录 | `deploy` / `restore` |
| 保存 / 应用预设 | `presets save NAME` / `presets apply PRESET_ID` |
| 导出 ZIP / 整合包 | `export MOD_ID [MORE_IDS] --output PATH` |
| 导入整合包 | `import-pack PATH` |

ID 从 `list` 或 `presets list` 获取；资产路径从 `info.data.assets[].relative_path` 获取。不要根据显示名称猜 ID，不要把 UI 的“第 N 顺位”当成 CLI 优先级数值。

## 3. 可靠读取结果

自动化优先使用 `--json`。输出是 **NDJSON**：每行一个 JSON 对象，前面可能有 `event: "progress"`，最后一行才是 `ok` 与 `data` 或 `error`。不要对整个 stdout 执行一次 JSON 解析。

- 返回码 `0`：命令正常返回；仍需检查业务结果。
- 返回码 `1`：执行失败，读取最终 `error.code` / `error.message`。
- 返回码 `2`：参数解析失败或未指定命令；错误可能仅出现在 stderr，没有 JSON。
- 部署：同时检查 `data.success`、`data.result.failed_files`、`warnings` 与 `errors`。`ok: true` 和退出码 0 不保证每个文件部署成功。
- 诊断：`data.healthy: false` 是有效的诊断结果；报告 `data.health.items` 的问题和修复建议。
- 列表：检查 `data.failures`，避免把扫描失败的模组误报成不存在。
- `--quiet` 不会抑制 JSON 进度事件。

附带 [scripts/run_smm.py](scripts/run_smm.py) 使用参数数组执行 CLI、解析每行 JSON，并输出单个汇总对象；不经过 shell，不修改 SMM 设置，不自动添加 `--yes` 或 `--overwrite`。调用路径相对于本技能目录：

```powershell
python "SKILL_DIR/scripts/run_smm.py" --exe "C:/Tools/SMM/smm.exe" -- --staging "D:/SMM staging" list
python "SKILL_DIR/scripts/run_smm.py" --exe "C:/Tools/SMM/smm.exe" -- --staging "D:/SMM staging" --game-dir "D:/SteamLibrary/steamapps/common/Sekiro" plan
```

将 `SKILL_DIR` 替换为本技能的实际绝对路径。不使用脚本也可以直接运行 CLI；PowerShell 中用 `& "C:/Tools/SMM/smm.exe" ...` 调用带空格的可执行路径。使用 Bash 时给每个路径加引号，或通过进程 API 的参数数组调用。

## 4. 按任务执行

### 导入并检查

1. 查看 `env`、`list`，确定输入路径及目标暂存区。
2. 执行 `import PATH --dry-run`，核对推导的 ID、资产数、类别和优先级。
3. 用户已要求导入且预览合理时，执行同参数的实际导入。若 ID 冲突，提供保留现有模组、换 ID 或覆盖的选择；仅在明确允许替换时添加 `--overwrite`。
4. 用 `info` 和 `conflicts` 验证。多份独立压缩包逐个导入；当前 CLI 的 `import` 只接受一个路径。
5. 用户仅要求导入时，到此结束，不自动部署游戏。

### 分析冲突并部署

1. 执行 `doctor`、`list`、`conflicts`；需要目标路径和胜出文件映射时执行 `plan`。
2. 每个冲突说明资产路径、级别、胜出 ID 和被覆盖 ID。优先级数值小者胜出，同值时按 ID 升序；禁用模组及禁用资产不参与部署。
3. 涉及 `gameparam.parambnd.dcx` 等严重冲突时，不自动选择用户偏好的整套玩法。通过优先级或禁用模组/资产解决覆盖；SMM 不会合并参数表。
4. 用户已要求部署、路径和覆盖意图明确后，先检查 `plan` 再执行 `deploy`。当前实现不会自动阻止严重冲突，不要依赖注释或 `--yes` 做保护。
5. 检查文件数、硬链接数、复制数和失败数。跨卷或硬链接失败会回退为复制；不能保证所有部署都是零拷贝。

`--profile` 只是部署计划/清单的标签，不会自动加载同名预设。加载组合必须先 `presets apply PRESET_ID`，再检查 `plan`。

### 预设和分发

- `presets save NAME` 保存当前启用模组的 ID 与优先级；不包含模组文件，也不单独快照资产开关。
- 应用预设会启用列出的模组、恢复其优先级，并禁用其余模组；应用后核对 `list` 和 `plan`。
- 单个 ID、不带 `--name`：导出 ZIP。多个 ID，或单个 ID 带 `--name`：导出 `.smmpack`。
- 导出先从 `list` 选择真实 ID，显式指定输出文件；默认导出不等于“当前所有启用模组”。
- `import-pack` 导入文件到暂存区，不等于应用预设或部署。用 `list` 检查后再执行用户要求的后续操作。

### 清理与删除

- `disable` 保留暂存文件；`remove` 永久删除整个模组目录。明确授权删除后才添加 `--yes`，以免自动化等待交互输入。
- **当前 `restore` 会遍历并删除目标 `mods/` 中清单之外的普通文件及符号链接。** 不要描述为“只删 SMM 追踪文件”。清理前检查目标范围；若存在用户未授权删除的手工模组，先备份或确认清理范围。
- `deploy` 可能覆盖同路径的手工文件；不要覆盖用户未授权替换的文件。硬链接部署后修改目标文件也可能修改暂存源文件，避免原地编辑已部署资产。
- `setup-engine` 会写入游戏目录的钩子/INI；先看诊断，只有任务授权修复时才执行。不自动下载或安装第三方 DLL。

## 5. 回复与验证

用用户的语言简洁报告实际结果：操作范围、ID/路径、执行或预览、文件/冲突计数、失败和必要下一步。命令未执行时明确标注“建议命令”，不要编造成功输出。

生成脚本或集成程序时按 NDJSON 协议处理结果、保存 stderr，并对超时和非零返回码显式处理。测试使用临时暂存区、临时游戏根目录和隔离的 `SMM_SETTINGS_FILE`；不对真实游戏运行测试性 `deploy`、`restore` 或 `setup-engine`。
