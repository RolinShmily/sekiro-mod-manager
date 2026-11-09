# 错误码与故障定位

核心只抛一种异常：`smm::SmmError`（`src/core/include/smm/error.hpp`）。它带三样东西：

```cpp
SmmError(ErrorCode code, std::string message, fs::path path = {});
```

- `code`：稳定枚举，`to_string()` 是进 JSON 的字符串，**只能新增，不能改名**。
- `message`：英文、面向开发者与脚本；`what()` 会追加 `[path: ...]`。
- `path`：出错的具体文件或目录；没有就为空。

## 全表（`src/core/src/error.cpp` 的 `kNames`）

| `code` | 语义 | 典型触发点 |
|---|---|---|
| `io` | 文件系统 / OS 级失败 | 无法创建目录、无法读文件、`settings.json` 写入失败 |
| `json` | JSON 解析或序列化失败 | 部署清单畸形、磁盘上 JSON 结构不符 |
| `metadata_not_found` | 模组目录里既没有 `.smm_mod.json` 也没有 `mod.json` | `load_mod_info()` |
| `invalid_metadata` | 元数据存在但形状或取值非法 | 预设文件损坏、字段类型错误 |
| `normalization_error` | 目录不包含可识别的 Sekiro 资源 | 压缩包解出来只有广告或安装器 |
| `no_assets_found` | 目录结构认出来了，但没有可部署资源 | 只有 README/截图 |
| `deploy_error` | 计划或目标校验失败 | 目标目录是磁盘根、路径不可写 |
| `mod_not_found` | 暂存区里没有该 id | `info` / `enable` / `priority` 等 |
| `mod_already_exists` | 导入目标 id 已被占用 | `import` 未加 `--overwrite` |
| `archive` | 压缩容器级失败 | 文件截断、CRC 校验失败 |
| `extraction_error` | 解压到一半失败 | 7z/rar 外部工具返回非零、缺少工具 |
| `unsupported_archive` | 容器格式不认识 | 改了扩展名的非压缩文件 |
| `environment_error` | 游戏 / ModEngine 环境不成立 | 找不到游戏目录、目录不存在 |
| `export_error` | 打包导出失败 | 输出路径不可写、磁盘满 |
| `mod_engine_source_missing` | 没有可安装的 ModEngine payload | `setup-engine` 且本机没有 `dinput8.dll` |
| `invalid_argument` | 调用参数非法 | 未知命令/选项、优先级不是整数、缺少 `--yes` |
| `cancelled` | 用户取消 | GUI 取消部署/导入/导出 |

## 传播路径

```
core 抛 SmmError
  ├── CLI：emit_error() → {"ok":false,"error":{"code","message"}}，退出码 1
  └── GUI：TaskRunner 捕获 → "code: message" → GuiController::reportFailure()
```

- CLI 的解析阶段（未知命令、未知选项、位置参数数量）在命令执行前就失败，退出码 **2**，没有 JSON 信封，只有 stderr 一行。
- 命令执行阶段的 `invalid_argument` 也是退出码 **1**：信封已经打开，按运行失败处理。
- GUI 里 `Cancelled` 被单独识别，走 `completion.cancelled`，界面显示"已取消"而不是红色错误。
- 未预期异常（`std::exception`）在 CLI 里被折算成 `io`，在 GUI 里保留 `what()` 文本。

## 按错误码排查

### `mod_not_found`

1. `smm list --json | jq '.data.mods[].id'` 看真实 id（id 是目录名，不是显示名）。
2. 核心会做一次大小写不敏感回退，但仍找不到就是真没有。
3. GUI 里点的是模型行，模型内容来自上一次扫描；写操作后没刷新时先 `refreshAll()`。

### `metadata_not_found` / `invalid_metadata`

- 模组目录必须直接包含 `.smm_mod.json` 或 `mod.json`（不是再套一层）。
- `invalid_metadata` 不会静默丢数据：预设文件解析失败时原文件保持不动，修好再用。
- 手工编辑元数据后，注意 `id` 与目录名不一致会让命令按目录名回退。

### `normalization_error` / `no_assets_found`

- 前者是"没认出这是只狼模组"，后者是"认出来了但没东西可部署"。
- 用 `smm import <path> --dry-run` 先看 `asset_count` 与 `ignored_count`，不写盘。
- 签名表在 `src/core/include/smm/normalizer.hpp`（`FILE_SIGNATURES`），新增扩展名要同时补测试。

### `unsupported_archive` / `extraction_error` / `archive`

- `.zip` 走内置 miniz，不依赖外部工具。
- `.7z` / `.rar` 依赖外部 `7za`/`7zr` 或 `UnRAR`/`WinRAR`；找不到时是 `extraction_error`，错误信息里带搜索过的路径。
- `archive` 表示容器本身坏了，重新下载通常比修更快。

### `mod_engine_source_missing`

- SMM 不分发 ModEngine。把合法的 `dinput8.dll`（> `MOD_ENGINE_DLL_MIN_SIZE`，即 10 000 字节）放到暂存目录（或其 `mod-engine/`、`mod-engine-0.1.16/` 子目录）、SMM 可执行文件旁的 `tools/mod-engine/`，或当前工作目录，再执行 `setup-engine`（`dinput8_candidates()`）；游戏目录是目标位置，不会被当作来源搜索。
- 体检里的 `dinput8` 项会把"文件存在但太小"标成 warning，那就是占位物。

### `deploy_error`

- 目标目录不能是空路径或磁盘根；`resolve_mods_dir()` 会把游戏根换成 `<game>/mods`。
- 跨卷时硬链接非法，会退化成复制并写 warning，不算失败。
- 单个文件失败不影响其他文件；看 `result.errors[]` 的 `{path,message}`。

### `cancelled`

- 只在 GUI 出现（CLI 没有取消入口）。
- 取消点位于文件之间与导入路径之间，因此可能有一小段收尾工作；下一次部署或还原会清掉半成品。

## 新增错误码

1. `enum class ErrorCode` 加一项（`error.hpp`）。
2. `kNames` 加 `{ErrorCode::X, "x_name"}`，数组长度常量同步改（`error.cpp`）。
3. 在抛出处选最贴近的码，不要新造"万能码"；能用 `invalid_argument` 表达的参数问题不要用 `io`。
4. 更新本页表格与 [json-contract.md](json-contract.md) 的错误信封示例。

改名或删除会让按 `code` 分支的脚本静默走错分支——这是唯一被明确禁止的改动。
