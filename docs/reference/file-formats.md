# 磁盘格式

SMM 只认自己写下的几份文件。手工编辑是允许的，但要遵守这里的字段约定；破坏结构会让对应命令失败，而且失败是显式的（不会静默丢数据）。

## 暂存区布局

```
staging/
├─ .smm_presets.json                  预设集合（可选；没有就是空列表）
└─ <mod-id>/                          一个模组一个目录，目录名就是 id
   ├─ .smm_mod.json                   权威元数据（SMM 写）
   ├─ preview.webp                    预览图（可选）
   ├─ .smm_source/                    原始下载备份（可选）
   │  └─ mod.zip
   └─ parts/ chr/ param/ ...          规范化后的游戏资源
```

- 只有直接包含 `.smm_mod.json`（或历史遗留的 `mod.json`）的目录才被当作模组。
- 规范化后的资源路径就是部署时的目标相对路径，见 [../architecture/data-flows.md](../architecture/data-flows.md)。
- `.smm_source/` 保存未改动的原始下载，导出时默认不包含；它的用途是导出原始包与事后排查，再次导入同一个文件仍会重新解压（`src/core/src/importer.cpp`）。

## `.smm_mod.json`

SMM 的唯一权威元数据。写入是原子的：先写 `._smm_mod.json.tmp`，再 `rename`；`rename` 失败才回退为直写（`ModManager::save_mod_info`）。

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `id` | string | 是 | 与目录名一致；缺失时回退为目录名 |
| `name` | string | 是 | 显示名，可为中文 |
| `version` | string | 否 | 作者给的版本号，原样保留 |
| `author` | string | 否 | |
| `category` | string | 否 | 自由文本；缺省 `custom`，规范集合见 `ModCategory` |
| `enabled` | bool | 否 | 缺省 `true` |
| `priority` | number | 否 | 缺省 `100`；**数字越小越优先** |
| `tags` | string[] | 否 | 缺省空数组 |
| `disabled_assets` | string[] | 否 | 被用户单独关掉的资源相对路径 |
| `description` | string | 否 | 缺失时整个键不写 |
| `homepage` | string | 否 | |
| `source_url` | string | 否 | 上游下载地址 |
| `license` | string | 否 | |
| `preview_image` | string | 否 | 通常是 `preview.webp`；读取时兼容旧键 `preview` |

- `root_path` 永远不落盘：它是本机路径，只在运行期存在。
- 读取优先 `.smm_mod.json`；只有它不存在时才读 `mod.json`，读完立即以 `.smm_mod.json` 重写并删除旧文件（`src/core/src/loader.cpp`），因此**不要让外部工具持续写 `mod.json`**。
- `enabled` 只认 JSON 布尔值；字符串 `"true"`、数字 `1` 都会被当成“未提供”而取缺省值 `true`（`types.cpp` 的 `read_bool`）。

## `mod.json`（历史遗留）

早期版本与生态工具使用的文件名。当前行为：

1. `.smm_mod.json` 存在 → 直接用它，`mod.json` 被忽略。
2. 只有 `mod.json` → 读它，随后立即以 `.smm_mod.json` 重写并删除 `mod.json`。

所以"只改 `mod.json`"在已有 `.smm_mod.json` 的模组上不会生效。

## `.smm_presets.json`

位于暂存区根目录，内容是一个 JSON 数组。没有文件或文件为空 → 空列表；解析失败 → `invalid_metadata`（不静默丢弃）。

```json
[{"id":"preset_boss_rush_1760000000","name":"Boss Rush","description":"...",
  "created_at":1760000000,"updated_at":1760000000,
  "mods":[{"mod_id":"mod-a","priority":10},{"mod_id":"mod-b","priority":20}]}]
```

| 字段 | 说明 |
|---|---|
| `id` | `preset_<slug>_<创建秒>`，slug 由名称转小写、非字母数字换成 `_` |
| `name` | 显示名；`save` 只按 `id` 去重，同名会生成新的 `id`（`PresetManager::find_by_name` 仅供调用方查重） |
| `description` / `name_en` / `description_en` | 可选，缺失时整键不写 |
| `mods[]` | 按 priority 升序；`apply` 时按此顺序写回 |

`apply` 的语义是"启用列表内的、禁用列表外的"，因此新增的模组不会自动加入旧预设。

## `.smm_manifest.json`（部署清单）

写在游戏侧的目标目录（通常是 `<Sekiro>/mods/`），记录 SMM 到底放了哪些文件，供下一次部署与 `restore` 精确回收。

```json
{"version":1,"profile":"default","deployed_at":1760000000,
 "files":["parts\\wp_a_0300.partsbnd.dcx"],
 "total_bytes":123456789}
```

- `files` 是相对目标目录的路径；`restore` 只删这些，外加清理空目录。
- 清单畸形 → 抛 `json`，命令失败；旧清单不可读时会警告"可能有残留"。
- 文件缺失（首次部署前）不是错误，视为空清单。
- `total_bytes` 是硬链接节省下来的字节数，供 `list`/界面显示遥测。

## `.smmpack`（模组集合容器）

- 实质是普通 ZIP，扩展名 `.smmpack`；导出的单模组包是 `.zip`，两者内部布局相同。
- 根目录必须有 `smm_pack.json`（允许被嵌在一层子目录里）。

```json
{"format_version":1,"name":"Boss Rush","version":"1.0.0","created_at":1760000000,
 "author":"...","description":"...",
 "mods":[{"id":"mod-a","name":"...","version":"1.0.0","author":"...","category":"weapon_skin",
          "priority":10,"enabled":true,"source_url":"...","homepage":"...","description":"...",
          "file_count":2,"total_bytes":117}]}
```

- `author`、`description` 以及每个模组的 `source_url` / `homepage` / `description` 都是可选的（缺失整键省略）。
- `file_count` 与 `total_bytes` 只是信息，导入时以实际文件为准。
- **清单对 `priority`、`enabled`、`source_url` 有权威性**：它们记录的是策展决定，不是文件属性。
- `format_version` 目前只认 `1`。

## `settings.json`（CLI 配置）

| 平台 | 路径 |
|---|---|
| Windows | `%APPDATA%\SMM\settings.json`（无 `APPDATA` 时用 `%LOCALAPPDATA%\SMM`） |
| POSIX | `~/.config/smm/settings.json` |
| 覆盖 | 环境变量 `SMM_SETTINGS_FILE` 指定完整路径 |

```json
{"staging_dir":"D:\\SMM\\staging","game_dir":"D:\\SteamLibrary\\steamapps\\common\\Sekiro"}
```

- 只有这两个键，由 `smm config` 写入（缩进 2）。
- 文件损坏不会让所有命令失败：解析失败按"没有配置"处理，`smm env` 会显示解析后的真实路径。
- GUI 不使用这个文件，它写 QSettings（`HKCU\Software\SekiroModManager\SMM`）。

## `modengine.ini`

位于游戏根目录（`sekiro.exe` 旁边）。`setup-engine` 只改 SMM 拥有的键，其余内容原样保留：

| 键 | 写入值 |
|---|---|
| `enabled` | `1` |
| `useModOverrideDirectory` | `1` |
| `loadLooseParams` | `1` |
| `modOverrideDirectory` | 目标 `mods` 目录（以 `game_dir / "mods"` 传入；写入时若值不以 `\` 或 `/` 开头会自动补一个 `\`） |

- 实现是先解析全文再按行重写（`patch_modengine_ini_text`），注释、未知键与顺序都不会丢。
- 文件不存在时用 `render_default_modengine_ini()` 生成一份最小可用配置。
- 体检会分别检查这四个键（`modengine-ini` / `modengine-enabled` / `modengine-override` / `modengine-loose-params`）。

## 预览图

- 统一命名 `preview.webp`，最长边 1920×1080、质量 82（`ModManager::compress_image_to_webp`）。
- 已经是 `preview.webp` 且小于 350 KB 时不重复压缩。
- 元数据没有 `preview_image` 时按候选名自动探测：`preview.png/jpg/jpeg/webp`、`cover.png/jpg/jpeg/webp`、`banner.png/jpg`。
- 设置新预览图后，模组目录里其它旧背景图会被清理，只留 `preview.webp`。

## 编码与路径

- 文件名、JSON 内容一律 UTF-8；Windows 上用原生 API 转换，避免代码页问题（`path_to_utf8` / `utf8_to_path`）。
- 资源相对路径在元数据与计划里统一用反斜杠（本机分隔符），比较前不要自己拼字符串。
- 规范化只影响"资源放在哪"，不改文件名大小写。
