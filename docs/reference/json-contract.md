# JSON 契约（`--json`）

`--json` 模式是给程序用的：**stdout 上一行一个 JSON 对象**，没有表格、没有颜色、没有空行噪声。实现见 `src/cli/src/json_output.cpp`。

## 信封

```json
{"ok":true,"command":"list","data":{"staging_dir":"D:\\SMM\\staging","count":2,"enabled_count":2,"mods":[],"failures":[]}}
{"ok":false,"command":"info","error":{"code":"mod_not_found","message":"Mod 'ghost' not found in staging [path: D:\\SMM\\staging]"}}
```

- `command` 是命令名，不是程序名；`ok` 为 `false` 时只有 `error` 块。
- `error.message` 来自 `SmmError::what()`，异常带路径时会追加 `[path: ...]`。
- 一行一个对象、逐行 `flush`：长任务执行期间调用方可以边读边更新界面，不必等进程退出。
- 键按字母序输出（`nlohmann::json` 的对象是有序映射），本页示例的字段顺序不是实际的字节顺序；按字段名取值，不要按位置解析。

## 进度事件

只有 `deploy`、`restore`、`export` 会在信封前输出进度：

```json
{"event":"progress","phase":"deploy","done":12,"total":37,"percent":32,"current":"parts\\wp_a_0300.partsbnd.dcx"}
```

`phase` 取值 `deploy` / `restore` / `export`；`total` 为 0 时不输出事件。进度事件没有 `ok` 字段，调用方按 `event` 分支即可。最后一个对象永远是信封，所以"读到最后一行拿结果"是安全的。

## data 形状

### list

```json
{"staging_dir":"...","count":2,"enabled_count":1,"mods":[<mod>],"failures":[{"path":"...","message":"..."}]}
```

`failures` 是无法读取的模组目录：命令仍然成功，但这些项会出现在界面与体检里。

### mod（`list.mods[]`、`info`、`enable`、`disable`、`enable-asset`、`disable-asset`、`set-preview`、`priority`）

```json
{"id":"kusabimaru-reaper","name":"...","version":"1.2.0","author":"...","category":"weapon_skin","description":null,"homepage":null,"source_url":null,"license":null,"enabled":true,"priority":30,"tags":[],"disabled_assets":["..."],"preview_image":"preview.webp","asset_count":2,"total_bytes":117,"root_path":"D:\\SMM\\staging\\kusabimaru-reaper"}
```

- 可空字段在这里**始终出现**，缺失时是 `null`，不是省略。
- `root_path` 是本机路径，不写进元数据，只在运行期有意义。
- 这是 `cmd_mods.cpp` 的 `mod_json()`，与核心的 `to_json(ModInfo)` **不是同一个投影**（见下）。

### info

`mod` 的全部字段 + `assets`：

```json
{"assets":[{"relative_path":"parts\\wp_a_0300.partsbnd.dcx","source_path":"...","file_size":58,"category":"parts","is_critical":false,"is_exclusive_slot":false,"enabled":true}]}
```

### remove

```json
{"removed":true,"mod":{<mod>}}
```

用户拒绝确认时 `removed` 为 `false`，`mod` 仍然是被指向的模组。

### import

```json
{"import":{"mod":{<核心 ModInfo>},"mod_dir":"...","asset_count":3,"ignored_count":2,"total_bytes":1234,"replaced_existing":false,"dry_run":false,"extracted_archive":true},"staging_dir":"..."}
```

### import-pack

```json
{"manifest":{<smm_pack.json>},"count":2,"mods":[{"import":{...}}],"staging_dir":"..."}
```

`mods[]` 的每一项仍带一层 `import` 键，和 `import` 命令的 `data.import` 完全同构。

### export

```json
{"output":"D:\\SMM\\exports\\kusabimaru-reaper.zip","as_pack":false,"mod_count":1,"include_source":false}
```

### plan / deploy

```json
{"profile":"default","target_dir":"D:\\...\\Sekiro\\mods","file_count":37,"mappings":[{"target_relative_path":"parts\\...","owner_mod_id":"mod-a","priority":10,"shadowed_mods":["mod-b"]}],"conflicts":[{"relative_path":"...","severity":"critical","winner_mod_id":"mod-a","shadowed_mod_ids":["mod-b"],"message":"..."}],"has_critical_conflict":false,"has_warning_conflict":true,"total_conflicts":1}
```

`deploy` 在此基础上追加执行结果：

```json
{"result":{"target_dir":"...","total_files":37,"hard_links_created":37,"copied_files":0,"skipped_files":0,"failed_files":0,"bytes_saved":123456789,"duration_ms":42,"success":true,"warnings":[],"errors":[{"path":"...","message":"..."}]},"success":true}
```

### restore

```json
{"target_dir":"...","result":{"target_dir":"...","removed_files":37,"removed_dirs":4,"duration_ms":18,"success":true,"warnings":[]}}
```

外层 `data.target_dir` 与 `result.target_dir` 是同一个值，保留两者只为与其他命令的字段名一致。

### conflicts

```json
{"has_critical_conflict":false,"has_warning_conflict":false,"total_conflicts":0,"records":[]}
```

### doctor

```json
{"health":{"overall":"degraded","game_dir":"...","staging_dir":"...","items":[{"id":"sekiro-exe","category":"game","title":"sekiro.exe","detail":"...","status":"ok","remediation":"..."}],"ok_count":6,"info_count":1,"warning_count":1,"error_count":0},"overall":"degraded","healthy":false}
```

`status`：`ok` / `info` / `warning` / `error`；`overall`：`healthy` / `degraded` / `action_required`。`remediation` 只在有建议时出现。`health` 是核心 `HealthReport` 的原样投影，外层 `overall` 与 `healthy` 是为方便判断重复出来的快捷字段。

### setup-engine

```json
{"engine":{"installed_dinput8":true,"dinput8_path":"...","ini_path":"...","source_used":"...","warnings":[]},"game_dir":"..."}
```

### presets

```json
{"action":"list","count":1,"presets":[{"id":"preset_boss_rush_1760000000","name":"Boss Rush","description":null,"created_at":1760000000,"updated_at":1760000000,"mod_count":2,"mods":[{"mod_id":"mod-a","priority":10}]}]}
```

`save` 与 `apply` 返回 `{"action":"save","preset":{...}}`；`delete` 返回 `{"action":"delete","preset_id":"..."}`。

### env / config

```json
{"settings_file":"C:\\Users\\<user>\\AppData\\Roaming\\SMM\\settings.json","staging_dir":"...","staging_exists":true,"game_dir":"...","game_dir_found":true,"game_dir_exists":true,"saved_staging_dir":"...","saved_game_dir":"..."}
```

`config` 只返回写入后的三个值：`settings_file`、`staging_dir`、`game_dir`。

## 两种模组投影

同一个模组在 JSON 里可能出现两种形状，调用方必须知道区别：

| 来源 | 特征 | 可选字段 |
|---|---|---|
| CLI `mod_json()`（`list`/`info`/`enable`…） | 含 `asset_count`、`total_bytes`、`root_path` | 缺失时为 `null` |
| 核心 `to_json(ModInfo)`（`import`、`import-pack` 里的 `mod`） | 不含统计与运行期字段 | 缺失时**整键省略** |

判别方法：看有没有 `asset_count`。这是历史遗留的不一致，收敛之前不要假设"所有 mod 对象字段相同"。

## 通用约定

- **时间**：UNIX 秒（`created_at`、`updated_at`、`deployed_at`），整数。
- **字节数**：整数，不做单位换算；人类可读的 `format_bytes()` 只出现在非 JSON 输出。
- **路径**：字符串，尽量保持调用方传入的写法（`--staging` 给正斜杠就回正斜杠），由 `resolve_mods_dir()` 拼出的子路径用本机分隔符。比较前先归一化。
- **排序**：`mods` 按优先级升序、同级按 id 升序；`conflicts` 按 critical → warning → info；`mappings` 按目标路径。
- **版本**：`smm --version` 输出 `smm (Sekiro Mod Manager) 0.4.3`，版本号来自 `project(... VERSION ...)`。

## 消费示例

```bash
# 拿启用模组的 id
smm list --json | jq -r '.data.mods[] | select(.enabled) | .id'

# 部署并跟踪进度
smm deploy --json | jq -c 'select(.event=="progress") | {done,total,percent}'
```

```powershell
# PowerShell：最后一行是信封
smm doctor --json | Select-Object -Last 1 | ConvertFrom-Json | ForEach-Object { $_.data.overall }
```

错误处理见 [errors.md](errors.md)；文件格式见 [file-formats.md](file-formats.md)。
