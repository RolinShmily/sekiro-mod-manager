# 命令行参考

```
smm [--json] [--staging <path>] [--game-dir <path>] <command> [args]
```

可执行文件名为 `smm`（CMake 目标 `smm`，别名 `smm-cli`）。命令表在 `src/cli/src/cli_parser.cpp` 的 `kCommands`，解析、`--help` 与误用提示都从这张表生成。

## 全局选项

| 选项 | 作用 |
|---|---|
| `--staging <path>` | 覆盖暂存目录。优先级高于 `settings.json` 与自动探测 |
| `--game-dir <path>` | 覆盖游戏目录 |
| `--json` | 一行一个 JSON 对象；stdout 不再有表格与颜色 |
| `--quiet` | 抑制进度输出（人类模式） |
| `--no-color` | 关闭 ANSI 颜色；输出不是终端时自动关闭 |
| `--yes` | 跳过交互确认（`remove` 必需，除非在终端里手工回答） |
| `--help` / `-h` | 无命令时打印总帮助；带命令时打印该命令的用法（不需要提供位置参数） |
| `--version` / `-V` | 打印 `smm (Sekiro Mod Manager) <版本>` |

`--opt value` 与 `--opt=value` 都接受。**未知选项或开关直接报错**，不会被静默忽略。

裸执行 `smm`（既无命令也无 `--help`）会打印总帮助并以退出码 `2` 结束：它是用法错误，不是“请求帮助”。`smm <command> --help` 会先校验选项合法性，再跳过位置参数检查，所以“这条命令要传什么”总问得出来。

## 退出码

| 码 | 含义 | 何时出现 |
|---|---|---|
| `0` | 成功 | 命令返回；`--help`、`--version` |
| `1` | 运行失败 | 命令执行中抛出 `SmmError`（含 `invalid_argument`、`cancelled`） |
| `2` | 用法错误 | 参数解析阶段失败：未知命令、未知选项、位置参数数量不对 |

`--json` 模式下失败也会输出 `{"ok":false,...}` 信封；但解析阶段的失败（退出码 2）连命令名都可能还没有，只在 stderr 打印一行错误。

## 命令一览

位置参数标 `...` 表示可变长度（至少一个）。

| 命令 | 位置参数 | 选项 | 开关 |
|---|---|---|---|
| `list` | — | — | — |
| `info` | `<mod-id>` | — | — |
| `enable` | `<mod-id>` | — | — |
| `disable` | `<mod-id>` | — | — |
| `enable-asset` | `<mod-id> <relative-path>` | — | — |
| `disable-asset` | `<mod-id> <relative-path>` | — | — |
| `set-preview` | `<mod-id> <image-path>` | — | — |
| `priority` | `<mod-id> <priority>` | — | — |
| `remove` | `<mod-id>` | — | `--yes` |
| `import` | `<path> ...` | `--id --name --priority --source-url` | `--overwrite --dry-run` |
| `import-pack` | `<pack-file>` | — | `--overwrite` |
| `export` | `<mod-id> ...` | `--output --name --description` | `--include-source` |
| `plan` | — | `--profile` | — |
| `deploy` | — | `--profile` | — |
| `restore` | — | — | — |
| `conflicts` | — | — | — |
| `doctor` | — | — | — |
| `setup-engine` | — | — | — |
| `presets` | `<action> ...` | `--description` | — |
| `env` | — | — | — |
| `config` | — | `--game-dir`（`--staging` 来自全局） | — |

## 命令细节

### 读取

```bash
smm list
smm list --json
smm info dream-of-the-damned
smm conflicts
smm env
```

- `list` 会先确保暂存目录存在（首次运行会创建），返回模组、启用数与无法读取的目录。
- `info` 的 `data` 就是 `list` 里单个模组的字段，外加 `assets` 数组。
- `conflicts` 只看**已启用**模组之间的路径碰撞，按 critical → warning → info 排序。
- `env` 打印解析后的路径与 `settings.json` 位置，不创建任何目录。

### 修改单个模组

```bash
smm enable kusabimaru-reaper
smm disable kusabimaru-reaper
smm priority kusabimaru-reaper 10      # 数字越小越优先
smm disable-asset kusabimaru-reaper parts\wp_a_0300.partsbnd.dcx
smm set-preview kusabimaru-reaper .\cover.png
smm remove kusabimaru-reaper --yes
```

- 这些命令的 `data` 是**修改后的完整模组投影**，脚本不需要再查一次。
- `set-preview` 会把图片压缩成 `preview.webp` 并写入元数据。
- `remove` 在非终端或 `--json` 模式下必须显式 `--yes`，否则报 `invalid_argument`；这是为了防止脚本卡在确认提示上。

### 导入与导出

```bash
smm import .\downloads\mod.zip
smm import .\part1.7z .\part2.7z --name "Combined"   # 多文件合并成一个模组
smm import .\mod.rar --id my-mod --priority 20 --dry-run
smm import-pack .\collection.smmpack
smm export kusabimaru-reaper --output .\exports
smm export mod-a mod-b --name "Boss Rush" --description "..." --output .\exports
smm export kusabimaru-reaper --include-source        # 连原始下载一起打包
```

- 单路径导入时 `--id/--name/--priority/--source-url` 生效；多路径会被合并成一个模组。
- `--dry-run` 只分析、不提交：暂存区不会出现新的模组目录（解压仍在临时目录里进行），`data.import.dry_run` 为 `true`。
- 导出单个模组且不带 `--name` 时产出 `.zip`；多个模组或带 `--name` 时产出 `.smmpack`。
- 默认**不**包含 `.smm_source`（原始下载），需要时加 `--include-source`。

### 计划与部署

```bash
smm plan
smm plan --json --profile default
smm deploy
smm restore
```

- `plan` 与 `deploy` 需要能解析出游戏目录，否则报 `environment_error`（退出码 1）。
- `deploy` 的人类输出与 JSON 都是"计划 + 执行结果"两段；`result` 里分别统计硬链接、复制、失败与节省字节。
- 部署、还原、导出在 `--json` 模式下会先输出若干 `progress` 事件，最后才是 `ok` 信封。
- `restore` 只删除 SMM 自己写进游戏目录的东西（依据 `.smm_manifest.json`），并清理空目录。

### 体检与环境

```bash
smm doctor
smm setup-engine
smm config --game-dir "D:\SteamLibrary\steamapps\common\Sekiro"
smm config --staging "D:\SMM\staging"
```

- `doctor` 只读：不修文件、不建目录。`overall` 为 `healthy` / `degraded` / `action_required`。
- `setup-engine` 需要用户自备 ModEngine 的 `dinput8.dll`：核心按 `dinput8_candidates()` 依次搜索暂存目录（含 `mod-engine/`、`mod-engine-0.1.16/` 子目录）、SMM 可执行文件旁的 `tools/mod-engine/` 与当前工作目录，游戏目录只是安装目标。找不到时抛 `mod_engine_source_missing`，SMM 不会伪造钩子。
- `config` 不带任何路径参数会报 `invalid_argument`——它不会无意义地重写文件。

### 预设

```bash
smm presets list
smm presets save "Boss Rush" --description "只留武器与特效"
smm presets apply preset_boss_rush_1760000000
smm presets delete preset_boss_rush_1760000000
```

- `save` 把"当前启用集合 + 各自优先级"固化成一条记录。
- `apply` 启用预设里的模组并写回优先级，**其余模组全部禁用**。
- `list` 返回 `mod_count` 与 `mods[]`，不需要再逐条 `info`。

## 解析规则

- 第一个非选项参数是命令，其后都是位置参数。
- 选项名在命令之前或之后都可以（`--json` 放在末尾也生效）。
- 位置参数数量按 `CommandSpec` 校验：固定数量必须精确匹配，标 `...` 的至少一个。
- `--help` / `-h` 在校验位置参数之前就短路返回：`smm export --help` 打印用法并退出 `0`，而不带 `--help` 的 `smm export` 因缺位置参数退出 `2`。
- 已知文案瑕疵：变参命令缺参数时的提示仍是 `The '<command>' command expects <minimum> argument(s) <positionals>`（`<positionals>` 里带着 `...`），措辞与"至少一个"的语义不符；行为本身正确，退出码仍是 `2`。
- `presets` 的第一位是动作（`list` / `save` / `apply` / `delete`），未知动作报 `invalid_argument` 并列出可选值。

新增命令需要改的位置见 [../architecture/layers.md](../architecture/layers.md)。
