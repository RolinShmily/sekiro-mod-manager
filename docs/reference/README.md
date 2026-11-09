# 参考分册

本分册是"查表"用的：命令、字段、错误码、磁盘格式。它只记录**已经实现**的行为，出处都在 `src/cli` 与 `src/core`。

## 分册目录

- [cli.md](cli.md) — 21 个命令、全局选项、退出码、解析规则
- [json-contract.md](json-contract.md) — `--json` 信封、进度事件、每个命令的 `data` 形状
- [errors.md](errors.md) — `ErrorCode` 全表、异常传播、按错误码排查
- [file-formats.md](file-formats.md) — 元数据、预设、部署清单、`.smmpack`、`settings.json`、`modengine.ini`
- [core-api.md](core-api.md) — `smm_core` 公开头文件索引与调用契约

## 谁在用这些契约

| 使用者 | 依赖的部分 |
|---|---|
| `smm` 命令行 | 全部；人类输出与 `--json` 输出共用同一份 `data` |
| GUI（`src/gui`） | 磁盘格式（直接链接 `smm_core`，不经过 CLI） |
| 外部脚本 / CI | `--json` 信封 + 退出码 |
| 文档与测试 | 错误码字符串、字段名 |

## 三条不变式

1. **错误码字符串不可改名。** `smm/error.hpp` 的枚举值经 `to_string()` 变成 JSON 里的 `code`，外部脚本按它分支。新增可以，改名等于破坏契约。
2. **`--json` 模式下 stdout 只有 JSON。** 一行一个对象，最后一个对象一定是 `ok` 信封；进度事件在它之前。
3. **路径用本机分隔符、UTF-8 编码。** Windows 上是反斜杠；需要跨平台比较时先归一化。

## 与其他分册的边界

- "为什么这样设计"在 [../architecture/decisions.md](../architecture/decisions.md)。
- "代码放在哪个目录"在 [../architecture/layers.md](../architecture/layers.md)。
- "怎么构建、怎么跑测试"在 [../development/](../development/README.md)。
