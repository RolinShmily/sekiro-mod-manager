# `smm_core` 公开 API 索引

头文件都在 `src/core/include/smm/`，命名空间 `smm`。这一层是纯 C++17：不链接 Qt、不打印、不读环境变量以外的全局状态（核心内部没有互斥量与线程，见 [../architecture/async-threading.md](../architecture/async-threading.md)）。

## 共同约定

- `smm::fs` 是 `std::filesystem` 的别名，`smm::json` 是 `nlohmann::json` 的别名（`types.hpp`）。
- 所有对外入口失败时抛 `smm::SmmError`（`error.hpp`），不返回错误码结构。
- 少量“探测型”函数改为返回空值而不抛异常，边界见下文「抛异常 / 不抛异常的边界」。
- 路径参数用 `fs::path`，字符串内容用 UTF-8 `std::string`；跨平台转换用 `path_to_utf8()` / `utf8_to_path()`。
- 结构体都是值类型，可以自由拷贝；`ModInfo::root_path` 是运行期字段，不落盘。

## 头文件一览

| 头文件 | 主要类型 | 关键入口 | 备注 |
|---|---|---|---|
| `types.hpp` | `ModInfo` `AssetEntry` `ConflictRecord` `ConflictReport` `DeployPlan` `DeployMapping` `ModPreset` `StagedMod` `ModCategory` `AssetCategory` | `to_json` / `from_json`、`format_bytes()`、`now_epoch_seconds()` | 所有磁盘与 JSON 契约的字段定义都在这里 |
| `error.hpp` | `ErrorCode` `SmmError` | `fail(code, message, path)` | 字符串名在 `error.cpp` 的 `kNames` |
| `normalizer.hpp` | `NormalizationResult` `FileSignature` `CANONICAL_DIRS` | `Normalizer::normalize_directory()` `find_canonical_root()` `classify_asset()` `normalize_relative_path()` `is_ignored_file()` | 任意目录 → 游戏内相对路径 |
| `loader.hpp` | `ScanOutcome` `ScanFailure` | `ModLoader::load_mod_info()` `scan_mod()` `scan_mods_directory()` `is_mod_directory()` | 单个模组读失败只进 `failures`，不中断整库扫描 |
| `manager.hpp` | —（返回 `ModInfo` / `StagedMod`） | `find_mod_dir` `set_mod_enabled` `set_mod_priority` `set_asset_enabled` `set_mod_preview` `rename_mod` `update_mod_info` `delete_mod` `save_mod_info` `get_mod_details` `reindex_priorities` `optimize_all_previews` | 元数据写入是原子的；`delete_mod` 会校验目标确实在暂存区内 |
| `conflict.hpp` | `ConflictReport` | `ConflictEngine::scan_conflicts()` `severity_for_path()` | 胜者 = 优先级最小，并列按 id 升序 |
| `deploy.hpp` | `DeployPlan` | `DeploymentPlanner::build_plan()` `build_plan_from_staging()` | 只生成计划，不碰磁盘 |
| `executor.hpp` | `DeployManifest` `DeployResult` `RestoreResult` `DeployProgress` | `execute_deploy()` `restore_deploy()` `resolve_mods_dir()` `resolve_destination_path()` `is_same_volume()` `create_hard_link()` `load_deploy_manifest()` `validate_target_dir()` | `DEPLOY_MANIFEST_NAME = ".smm_manifest.json"` |
| `extractor.hpp` | `ArchiveFormat` | `detect_archive_format()` `extract_archive()` `is_supported_archive()` `find_external_7z()` `find_external_unrar()` | zip 用内置 miniz；7z/rar 依赖外部工具 |
| `importer.hpp` | `ImportOptions` `ImportResult` | `import_mod()` `import_multiple_files_as_mod()` `collect_documentation_files()` `slugify()` `humanize_name()` `extract_version()` `infer_category()` `default_priority_for_category()` | 先在临时目录构建，最后整体换名进暂存区 |
| `exporter.hpp` | `ModPackManifest` `ModPackItem` `ExportOptions` `ModPackImportResult` | `export_single_mod()` `export_modpack()` `import_modpack()` | 容器是 ZIP，清单文件 `smm_pack.json` |
| `preset.hpp` | `ModPreset` `ModPresetEntry` | `PresetManager::list_presets` `save_preset` `create_preset_from_current` `apply_preset` `delete_preset` `find_by_name` | `PRESETS_FILE_NAME = ".smm_presets.json"` |
| `doctor.hpp` | `DiagnosticItem` `HealthReport` `ModEngineConfig` `EngineProvisionResult` | `diagnose_environment()` `provision_mod_engine()` `patch_or_create_modengine_ini()` `detect_game_dir()` `detect_staging_dir()` `is_process_elevated()` | `MOD_ENGINE_DLL_MIN_SIZE = 10'000`；体检只有五个 `id`（见 [data-flows.md](../architecture/data-flows.md)） |

## 抛异常 / 不抛异常的边界

| 函数 | 行为 |
|---|---|
| `load_deploy_manifest()` | 文件不存在 → `std::nullopt`；**存在但损坏** → 抛 `json` |
| `parse_modengine_ini()` / `parse_modengine_ini_text()` | 缺文件 → 返回默认配置；内容问题只体现在空的可选字段上，不抛异常 |
| `detect_game_dir()` / `detect_staging_dir()` | 找不到 → 空 `fs::path`，从不抛异常 |
| `is_same_volume()` / `hard_link_count()` | 查询失败 → `false` / `0`，不抛异常 |
| `find_external_7z()` / `find_external_unrar()` | 找不到 → 空路径 |
| 其余入口 | 失败一律抛 `SmmError` |

## 典型调用序列

### 扫描 → 计划 → 部署

```cpp
const smm::ScanOutcome outcome = smm::ModLoader::scan_mods_directory(staging);
const smm::DeployPlan plan = smm::DeploymentPlanner::build_plan("default", outcome.mods);
smm::validate_target_dir(target_mods_dir);
const smm::DeployResult result = smm::execute_deploy(plan, target_mods_dir, on_progress);
```

- `scan_mods_directory()` 已按"优先级升序、id 升序"排序，`build_plan()` 会再排一次，不依赖输入顺序。
- 计划里的 `mappings` 已按目标路径排序；`conflict_report` 一并生成，不需要单独再扫。

### 导入

```cpp
smm::ImportOptions options;
options.priority = 20;
options.overwrite = true;
const smm::ImportResult result = smm::import_mod(source, staging, options);
```

- 归档、嵌套归档、规范化、文档收集、预览压缩都在这一步完成。
- 目标 id 已存在且未开 `overwrite` → `mod_already_exists`。
- `dry_run` 为真时不写盘，但仍会解压到临时目录做分析。

### 体检

```cpp
const smm::HealthReport report = smm::diagnose_environment(game, staging, /*verify_deployment=*/true);
```

- 已扫描过暂存区的调用方传 `staging_scan` 指针，避免重复走目录。
- `overall` 由最差项推导；`items` 已按 error → warning → info → ok 排序。
- `is_process_elevated()` 只是报告当前进程的令牌状态，供界面提示；GUI 与 CLI 都通过 `requireAdministrator` 清单保证这一点（CLI 用 `/MANIFESTUAC`，见 [decisions.md](../architecture/decisions.md)）。

## 调用方禁止事项

1. **不要在核心外部复制业务规则。** 冲突裁决、优先级、规范化只有一份实现；GUI/CLI 只做投影与编排。
2. **不要并发写同一个暂存区。** 核心没有加锁，串行由调用方保证（GUI 用 `TaskRunner`，CLI 是单进程）。
3. **不要依赖 `.cpp` 里的内部函数。** `importer.cpp` 的 `materialize_mod`、`slugify` 的实现细节都不是 API；能用的都在 `include/smm/`。
4. **不要假设字段只增不减。** 反序列化用 `j.value(...)` 有默认值，序列化会省略空的 optional；跨版本读取时按"缺失即默认"处理。
5. **不要吞掉 `SmmError` 的 `code`。** 上层分支、日志与 JSON 契约都依赖它；只保留 `what()` 会丢掉机器可读部分。

返回值与字段的完整清单见 [json-contract.md](json-contract.md) 与 [file-formats.md](file-formats.md)。
