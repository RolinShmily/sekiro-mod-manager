# 四条主链路

每条链路都从用户动作开始，到磁盘状态结束。阅读时重点看"哪一步失败会留下什么"。

## 1. 导入：任意压缩包 → 规范化暂存目录

入口：`import_mod()` / `import_multiple_files_as_mod()`（`src/core/src/importer.cpp`），CLI `smm import`，GUI `smmBackend.importUrls()`。

```
源路径（目录 / .zip / .7z / .rar）
  → detect_archive_format()            扩展名优先，其次魔数
  → extract_archive()                  解压到临时 unpack_dir
       .zip  → 内置 miniz
       .7z   → 外部 7za/7zr，未安装则 ExtractionError
       .rar  → 外部 UnRAR/WinRAR
  → Normalizer::find_canonical_root()  找到真正承载 payload 的目录
  → Normalizer::normalize_directory()  签名表 → parts/ chr/ param/ menu/font/ ...
  → infer_category() + default_priority_for_category()
  → unpack_nested_archives()           包中包（zip 里再套 rar/7z）先就地展开
  → materialize_mod()                  在临时工作目录拼出 staging/<id> 的内容，收 README/LICENSE
  → 备份原始下载到工作目录/.smm_source/（随后随模组移入 staging）
  → normalize_preview_image()          预览图统一为 preview.webp
  → ModManager::save_mod_info()        原子写入 .smm_mod.json（顺带清掉历史 mod.json）
```

关键行为：

- `ImportOptions::dry_run` 为真时只分析、不写盘，返回结构里的 `dry_run` 与 `asset_count` 让调用方展示"将导入什么"。
- 目标 id 已存在时抛 `ModAlreadyExists`，除非 `overwrite` 为真；`overwrite` 会整体替换而不是合并。
- 单个导入失败不会污染暂存区：payload 先在临时目录构建，最后才换名进 `staging/<id>`。
- `.smm_source` 是"以后重新导入同一份原始下载"和事后排查的依据；导出时默认**不**包含它（`ExportOptions::include_source`）。

## 2. 部署：计划 → 硬链接 → 清单

入口：`DeploymentPlanner::build_plan()` + `execute_deploy()`（`src/core/src/deploy.cpp`、`executor.cpp`），CLI `smm plan` / `smm deploy`，GUI `smmBackend.deploy()`。

```
ModLoader::scan_mods_directory(staging)
  → 只要 enabled 的模组
  → 按 target_relative_path 分组
  → 胜者 = 优先级数字最小（并列时 mod id 升序）
  → ConflictReport（critical / warning / info）
  → DeployPlan (mappings 按目标路径排序)
  → validate_target_dir()
  → 读取旧 .smm_manifest.json，删除已不在计划中的文件
  → 逐文件落盘：
        同卷 → CreateHardLinkW
        跨卷或链接失败 → 复制，并记录 warning
  → 清理空目录
  → 重写 .smm_manifest.json（files / total_bytes / deployed_at）
```

- 目标目录由 `resolve_mods_dir(game_dir)` 决定：传游戏根目录得到 `<game>/mods`，传 `mods` 本身则原样返回。部署、还原、体检用同一个函数，因此不可能互相矛盾。
- `dinput8.dll` 与 `modengine.ini` 这类根目录文件走 `resolve_destination_path()`，落在 `sekiro.exe` 旁边而不是 `mods/` 下。
- 失败不是全有全无：单个文件失败进入 `DeployResult::errors`，其余继续，`is_success()` 以 `failed_files == 0` 判定。
- 硬链接的收益直接体现在 `bytes_saved`：这些字节没有第二份物理副本。

## 3. 预设：把"当前配置"固化成可切换状态

入口：`PresetManager`（`src/core/src/preset.cpp`），CLI `smm presets`，GUI 整合包页。

```
create_preset_from_current(staging, name, description)
  → 收集 enabled 的模组与优先级
  → 生成稳定 id（名称 + 创建秒）
  → 追加到 staging/.smm_presets.json

apply_preset(staging, preset_id)
  → 预设内模组：启用并写回存储的优先级
  → 其余模组：全部禁用
  → 返回应用后的预设
```

- 名称重复时 `find_by_name()` 返回已存在项，`create_preset_from_current()` 拒绝覆盖（"不悄悄改用户的存档"）。
- 预设是**快照**，不是规则：之后新增的模组不会被预设自动纳入，应用预设时会把它禁用，这是有意的（"切换整套配装"必须可预测）。
- 预设文件损坏时抛 `InvalidMetadata`，而不是当成空列表——静默丢弃用户存档是最糟糕的失败方式。

## 4. 诊断：读环境，不修环境

入口：`diagnose_environment()`、`provision_mod_engine()`（`src/core/src/doctor.cpp`），CLI `smm doctor` / `smm setup-engine`，GUI「环境检测」页与启动前检查。

体检被刻意压缩成五项，每项对应一个用户能直接处理的对象，`id` 是稳定键：

| id | category | 检查内容 |
|---|---|---|
| `game` | game | 游戏目录是否存在、`sekiro.exe` 是否在其中 |
| `modengine-hook` | modengine | 游戏根目录的 `dinput8.dll` 是否存在、是否小于 `MOD_ENGINE_DLL_MIN_SIZE`（占位物） |
| `modengine-ini` | modengine | `modengine.ini` 存在，且 `useModOverrideDirectory` / `modOverrideDirectory` 指向 `mods` |
| `staging` | staging | 暂存目录、模组数量与读取失败项、同卷/跨卷部署方式 |
| `deployment` | deployment | 当前启用文件与部署结果是否一致（`verify_deployment` 时） |

- `OverallHealth` 由最差项推导：`healthy` / `degraded` / `action_required`。
- `provision_mod_engine()` 只在用户提供 payload 时安装 `dinput8.dll`，SMM 自身不打包、不伪造钩子（见 [decisions.md](decisions.md)）。
- 不检查运行中的进程与 DLL 搜索策略：GUI 以管理员权限运行（见 [decisions.md](decisions.md)），`PreferSystem32` 这条启动链失败模式因此在源头消失，判定逻辑也随之从 core 里删掉。
- 体检结果只改变界面文案与启动前检查，不修改磁盘——修复永远是显式动作（`setup-engine`、`smm config`）。

## 失败时留下的状态

| 链路 | 中途失败 | 落盘结果 |
|---|---|---|
| 导入 | 解压/规范化失败 | 临时目录被丢弃，`staging/` 不变 |
| 导入 | 换名阶段失败 | 目标 id 不存在或整体替换，不会半份 |
| 部署 | 部分文件失败 | 已成功的文件在清单里，`errors` 记录失败项；下次部署会重算 |
| 还原 | 清单损坏 | 警告 + 全目录清扫，宁可多删自己写过的东西 |
| 预设 | JSON 损坏 | 抛 `InvalidMetadata`，原文件保留待人工修复 |
