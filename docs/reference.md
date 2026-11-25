# 技术参考

本文记录 Sekiro Mod Manager 已实现的磁盘文件契约、错误码全表、更新清单协议与 `smm_core` 头文件索引。

## 1. 磁盘目录与文件契约

### 1.1 暂存区结构

```
staging/
├── .smm_presets.json                整合包预设列表（可选）
└── <mod-id>/                        每个模组独立目录，目录名即 id
    ├── .smm_mod.json                权威元数据（原子写入）
    ├── preview.webp                 归一化预览图（可选，最大 1920×1080）
    ├── .smm_source/                 原始导入归档备份（可选，默认导出不包含）
    └── parts/ chr/ param/ menu/...  规范化后的只狼游戏资源路径
```

### 1.2 `.smm_mod.json`

| 字段 | 类型 | 必填 | 说明 |
| :--- | :--- | :--- | :--- |
| `id` | string | 是 | 与目录名一致；缺失时回退目录名 |
| `name` | string | 是 | 显示名称（支持中英文） |
| `version` | string | 否 | 模组版本号 |
| `author` | string | 否 | 原作者署名 |
| `category` | string | 否 | 分类标识，缺省 `custom` |
| `enabled` | bool | 否 | 是否启用，缺省 `true`（仅认布尔值） |
| `priority` | number | 否 | 裁决顺位，缺省 `100`；**数字越小越优先** |
| `tags` | string[] | 否 | 标签数组 |
| `disabled_assets` | string[] | 否 | 单独禁用的资产相对路径列表 |
| `description` / `homepage` / `source_url` / `license` | string | 否 | 简介与来源元数据 |
| `preview_image` | string | 否 | 预览图文件名（通常为 `preview.webp`，读取兼容 `preview`） |

> 历史 `mod.json` 仅作兼容读取：若目录下无 `.smm_mod.json` 但存在 `mod.json`，加载时会立即迁移为 `.smm_mod.json` 并删除旧文件。

### 1.3 `.smm_presets.json`

位于暂存区根目录的 JSON 数组，记录已保存的整合包预设：

```json
[
  {
    "id": "preset_boss_rush_1760000000",
    "name": "高难连战",
    "name_en": "Boss Rush",
    "description": "...",
    "description_en": "...",
    "created_at": 1760000000,
    "updated_at": 1760000000,
    "mods": [{ "mod_id": "mod-a", "priority": 10 }]
  }
]
```

应用预设时启用列表内模组并按 `priority` 写回，列表外模组统一禁用。

### 1.4 `.smm_manifest.json`（部署清单）

写入 `<Sekiro>/mods/.smm_manifest.json`，用于精确回收与空间节省遥测：

```json
{
  "version": 1,
  "profile": "default",
  "deployed_at": 1760000000,
  "files": ["parts\\wp_a_0300.partsbnd.dcx"],
  "total_bytes": 123456789
}
```

### 1.5 `.smmpack` 整合包容器

本质为标准 ZIP 归档（扩展名 `.smmpack`），根目录（或单层子目录）包含 `smm_pack.json`（`format_version: 1`）及各模组目录。清单中的 `priority`、`enabled`、`source_url` 具有权威性，导入时自动同步为模组与预设。

### 1.6 `modengine.ini`

位于游戏根目录 `sekiro.exe` 同级。SMM 按行修补 `enabled=1`、`useModOverrideDirectory=1`、`loadLooseParams=1` 与 `modOverrideDirectory="\mods"`，完整保留原文件注释与其他未知键值。SMM **不分发** `dinput8.dll`。

## 2. `ErrorCode` 全表

定义位于 `src/core/error.hpp` 与 `src/core/error.cpp`。错误码字符串为稳定契约，**只增不改**：

| `ErrorCode` | 字符串 | 语义 |
| :--- | :--- | :--- |
| `Io` | `io` | 文件系统 / OS 级读写错误 |
| `Json` | `json` | JSON 解析或序列化失败 |
| `MetadataNotFound` | `metadata_not_found` | 模组目录缺少 `.smm_mod.json` / `mod.json` |
| `InvalidMetadata` | `invalid_metadata` | 元数据或预设结构非法 |
| `NormalizationError` | `normalization_error` | 目录不含可识别的只狼资源结构 |
| `NoAssetsFound` | `no_assets_found` | 目录结构有效但无可部署文件 |
| `DeployError` | `deploy_error` | 目标路径非法或部署校验失败 |
| `ModNotFound` | `mod_not_found` | 暂存区中未找到对应模组 ID |
| `ModAlreadyExists` | `mod_already_exists` | 导入目标 ID 已存在且未允许覆盖 |
| `Archive` | `archive` | 压缩包损坏或 CRC 校验失败 |
| `ExtractionError` | `extraction_error` | 解压失败或缺少外部 `7z`/`unrar` 工具 |
| `UnsupportedArchive` | `unsupported_archive` | 无法识别的归档格式 |
| `EnvironmentError` | `environment_error` | 游戏目录或 ModEngine 环境异常 |
| `ExportError` | `export_error` | 导出打包失败 |
| `ModEngineSourceMissing` | `mod_engine_source_missing` | 未找到合法 `dinput8.dll`（> 10 KB） |
| `InvalidArgument` | `invalid_argument` | 调用参数非法 |
| `Cancelled` | `cancelled` | 用户主动取消后台任务 |

## 3. 桌面端更新与 Worker 路由契约

### 3.1 `latest.json` 发布清单

由 `.github/workflows/build.yml` 生成并随 GitHub Release 发布，由 `worker/index.ts` 在 `/releases/latest.json` 反代并重写资产链接为同域地址：

```json
{
  "version": "0.4.8",
  "tag": "v0.4.8",
  "published_at": "2026-11-23T00:00:00Z",
  "url": "https://sekiro.srprolin.com/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-gui.zip",
  "sha256": "<64-hex>",
  "gui_zip_url": "https://sekiro.srprolin.com/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-gui.zip",
  "gui_zip_sha256": "<64-hex>",
  "gui_zip_size": 22300000,
  "setup_exe_url": "https://sekiro.srprolin.com/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-setup.exe",
  "setup_exe_sha256": "<64-hex>",
  "setup_exe_size": 18500000,
  "notes_zh": "Sekiro Mod Manager v0.4.8 发布",
  "notes_en": "Sekiro Mod Manager v0.4.8 Release"
}
```

### 3.2 Worker 白名单路径

- `GET / HEAD /releases/latest.json`：拉取 GitHub Latest Release `latest.json` 并重写可信下载链接（`max-age=120`）。
- `GET / HEAD /releases/download/vX.Y.Z/sekiro-mod-manager-vX.Y.Z-windows-x64-(gui.zip|setup.exe)`：流式反代对应 GitHub Release 资产（`max-age=31536000, immutable`）。
- 其余 `/releases/*` 路径、携带查询参数或非 `GET`/`HEAD` 请求统一返回 `404` / `405`。

### 3.3 社区 `.smmpack` 尝鲜包清单（`website/public/packs.json`）

网站读取静态 `packs.json` 展示可选社区尝鲜整合包，支持优选 CDN 与 R2 源站双线路切换，并展示每个包内含模组的 Nexus Mods 原始链接与 SHA-256：

```json
{
  "schema_version": 1,
  "cdn_base_url": "https://cdn-r2-sekiro.srprolin.com",
  "origin_base_url": "https://r2-sekiro.srprolin.com",
  "packs": [
    {
      "id": "miyamoto-musashi",
      "file": "宫本武藏.smmpack",
      "object_key": "packs/miyamoto-musashi.smmpack",
      "size_bytes": 208348972,
      "sha256": "<64-hex>",
      "updated_at": "2026-10-06",
      "title_zh": "宫本武藏",
      "title_en": "Miyamoto Musashi",
      "summary_zh": "...",
      "summary_en": "...",
      "tags_zh": ["外观组合", "9 个模组"],
      "tags_en": ["Visual Pack", "9 Mods"],
      "mods": [
        { "name": "Miyamoto Musashi", "source_url": "https://www.nexusmods.com/sekiro/mods/1264" }
      ]
    }
  ]
}
```

## 4. `smm_core` 头文件索引

| 头文件 | 主要导出类型 / 函数 |
| :--- | :--- |
| `core/types.hpp` | `ModInfo`, `AssetEntry`, `StagedMod`, `ConflictReport`, `DeployPlan`, `ModPreset`, `path_to_utf8`, `utf8_to_path` |
| `core/error.hpp` | `ErrorCode`, `SmmError`, `to_string(ErrorCode)` |
| `core/normalizer.hpp` | `ModNormalizer`, `FILE_SIGNATURES` |
| `core/loader.hpp` | `ModLoader`, `ScanOutcome` |
| `core/manager.hpp` | `ModManager`（启停、优先级重排、改名、预览图压缩） |
| `core/conflict.hpp` | `ConflictEngine` |
| `core/deploy.hpp` | `DeploymentPlanner` |
| `core/executor.hpp` | `DeploymentExecutor`, `resolve_mods_dir`, `load_deploy_manifest`, `is_same_volume` |
| `core/extractor.hpp` | `ArchiveExtractor` |
| `core/importer.hpp` | `ModImporter` |
| `core/exporter.hpp` | `ModExporter` |
| `core/preset.hpp` | `PresetManager` |
| `core/doctor.hpp` | `diagnose_environment`, `setup_mod_engine`, `patch_modengine_ini_text` |
