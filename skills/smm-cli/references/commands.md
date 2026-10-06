# SMM CLI 命令参考

按仓库 `src/cli/src/cli_parser.cpp`、命令实现及 `smm.exe --help` 核对。当前源码为 0.3.6；本地已有二进制可能较旧，运行前用实际帮助验证。README 部分命令名尚未更新。

## 通用参数

```text
smm [--json] [--staging PATH] [--game-dir GAME_ROOT] COMMAND [ARGS]
```

| 参数 | 行为 |
| --- | --- |
| `--staging PATH` | 本次调用的模组暂存目录 |
| `--game-dir PATH` | 本次调用的只狼安装根目录，部署目标为 `PATH/mods` |
| `--json` | NDJSON 进度事件 + 最终结果 |
| `--quiet` | 隐藏人类模式进度；JSON 进度仍输出 |
| `--no-color` | 禁用终端颜色 |
| `--yes` | 跳过交互确认，不会解决冲突或授权其他行为 |
| `--help` / `-h` | 帮助文本 |
| `--version` / `-V` | 版本文本 |

全局参数可放在命令前后。值可用 `--staging=PATH` 或分开传入。路径有空格时按所在 shell 引号规则处理；使用参数数组时不要在参数内容里加引号。

**帮助陷阱**：当前解析器先验证必填位置参数，随后处理帮助。`smm info --help` 会因缺少 ID 返回 2；使用 `smm info PLACEHOLDER --help`。`presets` 用 `smm presets list --help`，`export` 用 `smm export PLACEHOLDER --help`。根帮助 `smm --help` 可直接运行。

## 命令表

下面的例子默认已附带 `--json` 与明确的路径覆盖。`MOD_ID`、`PRESET_ID`、`REL_PATH` 都应来自实际列表或详情。

| 命令 | 参数与结果 |
| --- | --- |
| `env` | `settings_file`, `staging_dir`, `game_dir`, `*_exists`, `saved_*` |
| `config --staging PATH --game-dir GAME_ROOT` | 持久化指定的一个或两个路径；不检查游戏有效性 |
| `doctor` | `data.overall`, `data.healthy`, `data.health.items`；有问题仍可能返回 0 |
| `setup-engine` | `data.engine` 的钩子、INI、来源、warnings；不提供下载地址参数 |
| `list` | `count`, `enabled_count`, `mods`, `failures` |
| `info MOD_ID` | 模组 metadata + `assets`，含 `relative_path`, `enabled`, `is_critical`, `is_exclusive_slot` |
| `enable MOD_ID` / `disable MOD_ID` | 修改启用状态，返回最新模组信息；不会自动部署 |
| `enable-asset MOD_ID REL_PATH` / `disable-asset MOD_ID REL_PATH` | 修改单文件状态，返回最新模组信息 |
| `set-preview MOD_ID IMAGE_PATH` | 设置本地预览图片，可能归一化/压缩为 WebP |
| `priority MOD_ID NUMBER` | 非负 uint32 数值，0–4294967295；小者先，默认生成值取决于类别 |
| `remove MOD_ID --yes` | 永久删除暂存模组目录；返回 `removed`, `mod` |
| `import PATH` | 仅一个输入，支持 ZIP、7z、RAR 或解压目录 |
| `conflicts` | `records`, `total_conflicts`, `has_critical_conflict`, `has_warning_conflict` |
| `plan [--profile NAME]` | 检查计划，含 `target_dir`, `file_count`, `mappings`, `conflicts`；需要存在的游戏根目录 |
| `deploy [--profile NAME]` | 部署相同计划；另有 `result` 与 `success` |
| `restore` | 清理游戏根目录下的 `mods`，返回 `target_dir` 和 `result`；计数、warnings、success 位于 `result` 中 |
| `presets list` | `action`, `count`, `presets` |
| `presets save NAME [--description TEXT]` | 保存当前启用组合；返回 `preset.id` |
| `presets apply PRESET_ID` | 恢复组合及优先级，禁用组合外模组 |
| `presets delete PRESET_ID` | 删除预设记录；不删除模组文件，无交互确认 |
| `export MOD_ID [MORE_IDS] --output PATH [--name NAME] [--description TEXT] [--no-source]` | 返回 `output`, `as_pack`, `mod_count`, `include_source` |
| `import-pack PACK_FILE [--overwrite]` | 导入 `.smmpack`，返回 `manifest`, `count`, `mods`, `staging_dir`；没有 dry-run 参数 |

`import` 可附加：`--id ID`、`--name TEXT`、`--priority NUMBER`、`--source-url URL`、`--overwrite`、`--dry-run`。dry-run 不安装目标模组，但会创建暂存/临时目录并解压检查内容。

## 覆盖及状态语义

- 模组与资产都启用时才参与冲突和部署。
- 优先级小者胜出，同值按 ID 升序。普通覆盖不会合并资源内容。
- `critical`：核心参数表；`warning`：排他性角色/武器槽位；`info`：一般资源。
- `plan` 的 `mappings` 含 `target_relative_path`, `owner_mod_id`, `priority`, `shadowed_mods`。
- `--profile NAME` 是标签，所有标签仍使用当前暂存启用状态；预设需要显式 `presets apply`。
- 预设只保存启用 ID 与优先级；资产开关保留在模组 metadata。
- 导出需要显式 ID。单个模组无 `--name` 为 ZIP；多个 ID 或带 `--name` 为整合包。
- 输出路径尽量明确带 `.zip` 或 `.smmpack`；导出可替换同名已有输出，先检查目标文件。
- 7z 需要外部 7-Zip，RAR 可使用 7-Zip 或 WinRAR；普通 ZIP 内置解压。

## 已核实的实现限制

1. 旧文档中的 `set-priority`、`pack-export`、`pack-import` 不是当前命令。使用 `priority`、`export`、`import-pack`。
2. 导出实现检查 `--include-source`，但解析器拒绝该参数，只声明 `--no-source`。当前默认不包含原始下载，不能承诺加 `--include-source` 可工作；报告限制，不绕过解析器。
3. 导入实现有多路径分支，但解析器要求恰好一个路径；不要照该分支生成多文件 `import` 命令。
4. 当前部署实现没有因严重冲突而提前拒绝，注释不能作为阻止执行的证据。
5. `restore` 在处理清单后还会遍历剩余文件/符号链接并删除；**无论清单是否正常，都可能清理未追踪文件**。
6. `list`、`info`、`plan` 等扫描过程可能把旧 `mod.json` 迁移成 `.smm_mod.json`，并探测/归一化预览图。属于查看操作，但并非绝对无磁盘副作用。明确要求完全不改动时先使用副本。
7. CLI `import-pack` 不会像 GUI 的导入流程一样额外创建可应用的预设；需要时显式保存当前启用组合。
8. CLI 与 GUI 分别使用 JSON 设置文件、QSettings，不能互相推断路径。

## JSON 协议

进度示例：

```json
{"event":"progress","phase":"deploy","done":1,"total":2,"percent":50,"current":"parts/wp_a_0300.partsbnd.dcx"}
```

最终成功：

```json
{"ok":true,"command":"list","data":{"count":0,"enabled_count":0,"mods":[],"failures":[],"staging_dir":"D:/SMM/staging"}}
```

执行失败：

```json
{"ok":false,"command":"info","error":{"code":"mod_not_found","message":"..."}}
```

参数解析失败可能没有 JSON，仅 stderr 与退出码 2。部署在返回 `ok: true` 时仍可能有 `data.success: false`；还原检查 `data.result.success`，诊断检查 `data.healthy`，删除检查 `data.removed`。

## 工作流示例

命令以占位符 `smm` 表示已找到的程序；替换路径和 ID 后执行。

```powershell
# 检查普通导入，然后根据结果安装；不会自动部署
smm --json --staging "D:/SMM/staging" import "D:/Downloads/katana.zip" --id katana --dry-run
smm --json --staging "D:/SMM/staging" import "D:/Downloads/katana.zip" --id katana
smm --json --staging "D:/SMM/staging" info katana
smm --json --staging "D:/SMM/staging" conflicts

# 显式调整覆盖意图，检查后部署
smm --json --staging "D:/SMM/staging" priority katana 10
smm --json --staging "D:/SMM/staging" disable-asset katana "parts/wp_a_0310.partsbnd.dcx"
smm --json --staging "D:/SMM/staging" --game-dir "D:/Games/Sekiro" plan
smm --json --staging "D:/SMM/staging" --game-dir "D:/Games/Sekiro" deploy

# 保存预设与分发是不同操作；PRESET_ID 取自实际返回
smm --json --staging "D:/SMM/staging" presets save "Weapons" --description "Katana setup"
smm --json --staging "D:/SMM/staging" presets list
smm --json --staging "D:/SMM/staging" presets apply PRESET_ID
smm --json --staging "D:/SMM/staging" export katana --name "Weapons" --output "D:/Exports/weapons.smmpack"
smm --json --staging "D:/SMM/second-staging" import-pack "D:/Exports/weapons.smmpack"
```
