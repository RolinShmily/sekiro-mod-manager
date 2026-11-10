# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html). This file is a human-readable
summary; `git log` remains the authoritative record.

## [0.4.6] - 2026-11-10

### Changed

- **The GUI no longer animates at all.** `HusTheme.animationEnabled` is pinned to `false` before the QML engine starts, and every `Behavior` / `*Animation` / `Transition` is gone from the QML: no page fades, card entrances, hover colour transitions or toast slides. The "reduce motion" setting, its `GuiController` property and its QSettings key were removed with it, so a theme switch is now just one `setDarkMode()` call with nothing to suspend and restore.
- **The settings page is a two-column form.** Each setting is a row with the label and its explanation on the left and the control aligned in one 340 px column on the right; groups are spaced 30 px apart, rows 18 px. The three icon-only path buttons became labelled buttons (Browse / Open / Auto-detect), and the motion card is gone.
- `scripts/local/capture_screenshots.ps1` follows the six-item navigation rail (the settings page was being captured as the diagnostics page) and retries each frame once, because `PrintWindow` can return the previous page's composited frame right after a switch.
- **The CLI requires administrator rights too** (`/MANIFESTUAC:level='requireAdministrator'`), matching the GUI. Deploy, restore and `modengine.ini` writes all land in the game directory, so the CLI was already failing with `Access is denied` whenever that directory lived under `Program Files`. Both entry points now share one permission model. Note the Windows rule this exposes: `CreateProcess` cannot raise UAC, so `smm` started from an unelevated cmd/PowerShell/VS Code terminal/Python `subprocess` fails with `ERROR_ELEVATION_REQUIRED` (740) and no prompt — run it from an elevated terminal, or via `start smm ...`. This supersedes the 0.4.4 note that the installer's `PATH` entry lets you "run smm from any terminal": the entry is still written, it just only resolves in an elevated terminal now.

### Removed

- The "reduce motion" switch, `GuiController::reducedMotion` and the `reducedMotion` QSettings key.

### 中文摘要

- **GUI 彻底不做动效**：QML 引擎创建前把 `HusTheme.animationEnabled` 固定为 `false`，QML 里的 `Behavior` / `*Animation` / `Transition` 全部删除——没有页面淡入、卡片入场、悬停颜色过渡与 Toast 位移。“减少动效”开关、对应的 `GuiController` 属性与 QSettings 键一并移除，换肤只剩一次 `setDarkMode()`。
- **设置页改成两列布局**：每个设置项是“标签 + 说明在左、控件在右”的一行，右列统一 340 px；分组间距 30 px、组内项间距 18 px。三个只有 tooltip 的路径图标按钮换成带文字的按钮（浏览 / 打开 / 自动探测），动效分组整张卡片删除。
- `scripts/local/capture_screenshots.ps1` 跟上六项导航（之前把设置页拍成了环境检测页），并给每页多拍一帧：`PrintWindow` 在切页瞬间可能返回上一页的合成帧。


### 中文摘要

- **CLI 也改为要求管理员权限**（`/MANIFESTUAC:level='requireAdministrator'`），与 GUI 一致。部署、还原、写 `modengine.ini` 都落在游戏目录，该目录在 `Program Files` 下时本来就会失败于 `Access is denied`；现在两个入口共用一套权限模型。需要知道 Windows 的这条规则：`CreateProcess` 弹不出 UAC，所以从非提权的 cmd / PowerShell / VS Code 终端 / Python `subprocess` 启动 `smm` 会直接返回 `ERROR_ELEVATION_REQUIRED`（740）且没有任何弹窗——请在提权终端里运行，或用 `start smm ...`。这条推翻了 0.4.4 里"装完即可在任意终端直接运行 `smm`"的说法：`PATH` 条目仍然会写入，只是在非提权终端里解析不到能启动的进程。

## [0.4.5] - 2026-11-09

### Changed

- The environment doctor is a page in the navigation rail (`DiagnosticsView`) instead of a modal: results, the repair action and "when checks run" stay on screen, and `Ctrl+4` / `Ctrl+R` jump there.
- The doctor now reports five items — game + `sekiro.exe`, the ModEngine hook, `modengine.ini`, staging, deployment — instead of one row per ini key and per running process. Chinese detail text is assembled in the GUI from those stable ids.
- Checks only run when the launcher opens, when Launch is pressed, and when "Check environment" is pressed. Other writes rescan the staging directory only, a changed path discards the stale report, and the 10 s process timer is gone.
- Pressing *Launch* now performs the check itself: the button is clickable unless a task is running, and a failing check blocks the launch and opens the diagnostics page.
- **The GUI requires administrator rights** (`requireAdministrator` manifest). Steam's `PreferSystem32` DLL policy made an unelevated launch chain load the system `DINPUT8.dll` instead of ModEngine's, which silently disabled every mod; the old workaround asked users to close Steam first. UAC is now shown once per start, and the runtime process/DLL policy diagnostics were removed with the failure mode.
- Theme switching applies in a single frame: colour transitions are suspended for the switch, the hero art and both calligraphy variants stay decoded (`sourceSize` + `asynchronous`), so changing the theme no longer decodes a 940×1920 PNG on the GUI thread.

### 中文摘要

- 环境诊断从弹窗改成导航分页（`DiagnosticsView`），检测结果、修复入口与「什么时候检测」都留在页面上；`Ctrl+4` / `Ctrl+R` 直达。
- 体检压缩成五项（游戏与 `sekiro.exe`、ModEngine 钩子、`modengine.ini`、暂存区、部署一致性），不再把 ini 每个键、每个进程各拆一行；中文文案由界面按稳定 `id` 拼装。
- 只在打开启动器、点击「启动只狼」、点击「检测环境」时体检；其他写操作只重扫暂存区，路径变更直接丢弃过期结果，10 秒定时器删除。
- 点击「启动只狼」本身就会先检测：按钮不再常灰，检测不过则阻止启动并跳到诊断页。
- **GUI 改为要求管理员权限**（`requireAdministrator` 清单）：非提权启动链会被 Steam 的 `PreferSystem32` 策略抢走系统 `DINPUT8.dll`，导致模组静默失效；原来的规避办法是让用户先关 Steam。现在每次启动由系统弹一次 UAC，运行态进程与 DLL 策略体检随失败模式一起删除。
- 主题切换一帧完成：切换期间暂停颜色过渡，首屏画卷与两版书法水印常驻内存（`sourceSize` + `asynchronous`），换肤不再在 GUI 线程解码 940×1920 的 PNG。

## [0.4.4] - 2026-11-09

### Added

- The setup installer offers to add the CLI to the current user's `PATH`, so `smm` works from any terminal; the uninstaller removes exactly that entry again and leaves other entries untouched.
- The portable bundle and the setup ship the agent skill: `skill/smm-cli.zip` (the same archive the website serves) plus an extracted copy for agents that read a folder.

### Changed

- The Windows installer asset is named `sekiro-mod-manager-<version>-windows-x64-setup.exe` (previously `...-gui-setup.exe`); the GUI and CLI zips keep their names. **Upgrade note:** clients up to 0.4.3 build that file name themselves, so their "download installer" button points at a missing asset until they update; the release page button still works.
- `dxil.dll` is pruned together with `dxcompiler.dll` (it is only meaningful beside it), bringing the Windows bundle to 53.5 MB on disk and 21.7 MB zipped.

### 中文摘要

- 安装程序可选把 CLI 写入当前用户 `PATH`，装完即可在任意终端直接运行 `smm`；卸载时精确移除该条目，不影响用户的其他 PATH 项。
- 便携包与安装包都带上智能体 Skill：`skill/smm-cli.zip`（与官网提供的是同一个压缩包）以及解压后的目录，便于直接拷进 agent 的 skills 目录。
- 安装包资源名改为 `...-windows-x64-setup.exe`（原为 `...-gui-setup.exe`）；GUI 与 CLI 的 zip 名称不变。**升级提示**：0.4.3 及更早的客户端会自行拼出旧文件名，它们的「下载安装程序」按钮会暂时 404，改用「前往发布页面」即可。
- 与 `dxcompiler.dll` 一同裁剪 `dxil.dll`（后者只在配套前者时有意义），Windows 便携包降到 53.5 MB（zip 21.7 MB）。

## [0.4.3] - 2026-11-09

### Added

- Settings, About and the version check are now pages in the navigation rail instead of modal dialogs; each group sits directly on the page background (icon, title, hairline, whitespace) instead of in a nested card.
- Screenshot gallery under `docs/design/` (seven WebP captures of a fictional demo library) and a screenshot section in both READMEs.
- Documentation set: `docs/architecture`, `docs/design`, `docs/development`, `docs/reference`, indexed from `docs/README.md`.
- `smm_gui` accepts `--staging=` and `--game-dir=` for a single run without touching saved settings.

### Changed

- The GUI backend is split by responsibility (adapters, controllers, models, providers, services); `smm_core` gained the rules that used to live in the GUI (`resolve_mods_dir`, `load_deploy_manifest`, `rename_mod`, `sanitize_mod_id`, `reindex_priorities`) and diagnostics reuse a pre-scanned staging directory.
- `import` accepts several paths and merges them, `export` documents `--include-source`, and `--help` works for commands with required arguments.
- Mod metadata is written once as `.smm_mod.json`; a legacy `mod.json` is read for compatibility and removed on the next save.
- Vendored libraries live next to their consumers (`src/core/third_party`, `src/gui/third_party`); development helpers moved to `scripts/`.

### Fixed

- The background-task badge kept a spin animation alive while hidden, repainting the window at ~150 fps and burning about 15% of one core while idle.
- The portable GUI bundle omitted `Qt6OpenGL.dll` even though `Qt6Quick.dll` imports it, so it could not start on a machine without that DLL.
- Mod previews are decoded at the size the card asks for instead of full resolution and then downscaled.

### Packaging

- Windows GUI bundle: 114.6 MB → 54.9 MB on disk and 60.6 MB → 22.3 MB zipped, with the VC++ runtime shipped as DLLs instead of the 25 MB redistributable.
- Pull request CI now builds the headless test suites and runs `ctest`.

### 中文摘要

- 「全局配置」「关于」与版本更新改为导航分页；分组直接落在页面背景上（图标、标题、细分隔线、留白），不再在背景里再套一层卡片。
- 新增 `docs/design/` 截图集（7 张 WebP，来自虚构演示库）与中英 README 的界面展示；新增 `docs/` 四套文档（架构 / 设计 / 开发 / 参考）。
- `smm_gui` 支持 `--staging=` / `--game-dir=` 单次运行覆盖，不写回已保存设置。
- GUI 后端按职责拆分（adapters / controllers / models / providers / services）；原来散在 GUI 的业务规则（部署目录解析、清单读取、改名、优先级重排）回到 `smm_core`，诊断复用同一次暂存扫描。
- `import` 支持多路径合并、`export` 的开关更正为 `--include-source`、带必填参数的命令也能用 `--help`。
- 元数据只写 `.smm_mod.json`，旧的 `mod.json` 仅兼容读取并在下次保存时清理。
- 修掉后台徽标隐藏后仍在旋转、导致空闲时约 15% 单核占用的空转；修掉便携版漏带 `Qt6OpenGL.dll` 导致无法启动的问题；预览图按卡片尺寸解码。
- 便携包体积 114.6 MB → 54.9 MB（zip 60.6 → 22.3 MB），VC++ 运行时改以 DLL 形式随包发布。
- PR CI 现在会构建并运行无界面测试套件。

## [0.4.2] - 2026-10-26

### Fixed

- Update the About dialog's displayed blog address and visit link to `https://blog.srprolin.com` so installed clients use the current domain after upgrading.

### Changed

- Update the official website links, canonical URL and Open Graph URL to `https://sekiro.srprolin.com/`.
- Explicitly disable Cloudflare `workers.dev` and version / preview URLs while retaining the existing dashboard-managed DNS and domain routing.

### 中文摘要

- 将客户端「关于」窗口中的博客显示地址和访问链接更新为 `https://blog.srprolin.com`，升级软件后即可使用新域名。
- 官网入口、canonical 和 Open Graph 页面地址统一为 `https://sekiro.srprolin.com/`。
- 显式关闭 Cloudflare `workers.dev` 及版本 / 预览 URL，沿用控制台已有的 DNS 和域名路由配置。

## [0.4.1] - 2026-10-16

### Changed

- Simplify the launch hub workbench by removing the redundant pre-launch check banner at the top, relying on the compact hero indicator and the dedicated sidebar running status card.

### 中文摘要

- 简化启动工作台页面结构：移除首页顶部的冗余「启动前检查」横幅，统一保留左侧导航栏底部的全景运行状态卡片及主画卷内的紧凑诊断指示。

## [0.4.0] - 2026-10-06

### Added

- Proactively surface mod, deployment and ModEngine diagnostics on the launch workbench. Verify the SMM/Steam/Sekiro `PreferSystem32Images` policy and the game's loaded local hook; require a clean preflight before launching, and recheck after deployment and process startup.

### Fixed

- Diagnose ModEngine's actual `[files] useModOverrideDirectory` switch and managed override target instead of treating the legacy `enabled` key as the loader switch. Compare deployed files with the current plan and flag stale manifest entries before launch.
- Prevent deploy-and-launch from deploying when preflight fails or starting Sekiro before asynchronous deployment has completed.

### 中文摘要

- 启动工作台主动展示模组、部署与 ModEngine 诊断；检查 SMM、Steam、只狼进程的 `PreferSystem32Images` 与本地钩子加载状态，诊断通过后才允许启动，并在部署和启动后复查。
- 按 ModEngine 实际读取的 `[files] useModOverrideDirectory` 检测配置目标，而非误把旧 `enabled` 键当作加载开关；核对部署文件并阻止使用过期部署启动。
- 修复“部署并启动”未等待异步部署完成、以及诊断异常时仍先部署的问题。

## [0.3.7] - 2026-10-06

### Added

- Search all installed Windows font families by name, with match counts, a bilingual preview and missing-font fallback hints. Recommend installed Noto Sans SC (思源黑体系列); preserve existing font selections.
- Persist a reduced-motion preference across HuskarUI controls, navigation, dialogs, notifications and mod reordering. Add keyboard activation and visible focus to navigation.
- Add the `skills/smm-cli` agent skill with source-verified workflows and a standard-library NDJSON runner.
- Publish a bilingual product website with real application screenshots and separate GUI installer, portable, CLI and Skill downloads.

### Changed

- Derive mod lists, conflicts and deployment plans from one staging scan. Batch enable/delete deduplicates IDs and refreshes once, with partial-failure reporting.
- Retain mod delegates when order and filter membership remain stable; reorder with row-move notifications. Debounce search and share rank options across cards.
- Load mod and preset pages on first visit and retain them afterward; skip hidden preview images and avoid full rescans for appearance-only settings.
- Refine page fades, modal entrance and notification movement, and reproduce HuskarUI button effects through the compatibility patch script.
- Derive CLI, update-check and About versions from the CMake project version; align Windows numeric and string file versions.

### Fixed

- Prevent bound switch updates from writing mod enabled state. Reload previews after image replacement while preserving card delegates.
- Keep unsaved theme choices when switching settings language, and respect reduced motion for button press scaling.

### 中文更新摘要

- 全量搜索 Windows 已安装字体，提供匹配数量、中英文预览与缺失字体提示；优先推荐已安装的 Noto Sans SC，保留原有字体选择。
- 新增可保存的减少动效设置，优化页面、弹窗、通知与卡片交互，并增加导航键盘操作。
- 模组、冲突和部署计划共用一次扫描，批量操作只刷新一次；保留卡片实例，页面首次访问才加载，外观设置不再触发全量扫描。
- 修复开关绑定误触发、预览更新及语言切换导致待保存主题重置的问题。
- 新增 smm-cli 智能体技能与双语产品网站，统一软件版本信息。

## [0.3.6] - 2026-10-05

### Changed

- **QtRendering Antialiasing & Color Fringing Elimination**: Switched global text rendering engine from DirectWrite ClearType (`NativeRendering`) to distance-field grayscale antialiasing (`QtRendering`). Completely eliminated red/blue subpixel chromatic aberration (color fringing) and broken pixel artifacts on dark backgrounds and transparent alpha viewports.
- **Default UI Font Upgrade to Microsoft YaHei UI**: Set default interface font to `Microsoft YaHei UI` across `main.cpp`, `Models.cpp`, and preferences. Established unified CJK/Western typography fallback chain (`Microsoft YaHei UI` ➔ `Segoe UI` ➔ `PingFang SC` ➔ `Noto Sans SC`), avoiding glyph fragmentation and fallback baseline jumps.
- **High-DPI PassThrough & 4x MSAA**: Enforced `PassThrough` high-DPI scaling policy and 4x MSAA surface format to prevent fractional pixel viewport blur and viewport jitter on 125%/150% Windows scaling.
- **Card Hierarchy & Layout Proportions**:
  - Elevated card title to `15px bold` for clearer focal point.
  - Eliminated jagged `11px` micro-text across the UI, normalizing all author, version, source, and footnote labels to `12px`.
  - Refined mod description to `13px` with `lineHeight: 1.25` for improved reading density.
  - Standardized toolbar action buttons to `32px` height, aligned search and category filter capsules to `30px`, and expanded grid cell height to `184px`.

## [0.3.5] - 2026-10-04

### Fixed

- **Mod Pack Preset Dialog Scoped Resolution**: Resolved critical `ReferenceError: packNameInput is not defined` in `SavePackModal` by lifting state properties (`packName`, `packDesc`, `packNameEn`, `packDescEn`) to root modal scope and synchronizing text inputs via bindings, fixing non-responsive "Save Preset" action and enabling Enter key quick submission.
- **Empty Presets File Resilience**: Added 0-byte file guard in `PresetManager::list_presets` to prevent `[json.exception.parse_error.101]` parser crashes, and ensured pre-serialization before file truncation in `save_preset` and `delete_preset`.
- **Windows CLI Unicode Argument Parsing**: Converted command-line arguments to UTF-8 using `CommandLineToArgvW` on Windows, eliminating JSON encoding failures when passing non-ASCII names via CLI.

## [0.3.4] - 2026-10-03

### Added

- **Native Windows Typography Settings & Segoe UI Default**: Re-engineered UI typography system to eliminate excessive bold rendering (`Normal 400` baseline with `14px` standard desktop scale). Added native Windows font selector in Global Settings with Segoe UI (default recommended), Microsoft YaHei UI, DengXian, SimHei, KaiTi, and Cascadia Code support.
- **HuskarUI Dual Theme Support**: Added theme mode switcher in Global Settings with full support for HuskarUI Dark, Light, and Follow-System themes with real-time palette adaptation.
- **Unified `preview.webp` Backdrop Specification**: All mod preview backdrop images (whether imported, uploaded, or replaced) are now strictly normalized, compressed, and saved exclusively as `preview.webp`.
- **WIC Hardware-Accelerated Image Provider (`ModPreviewImageProvider`)**: Built custom `image://modpreview/` protocol in C++ leveraging native Windows Imaging Component (WIC) hardware decoding, async on-demand downsampling (480x270 for cards, 960x540 for drawer), and LRU memory cache, saving over 90% GPU texture memory.
- **Batch Mod Backdrop WebP Migration**: Automatically batch-converted all existing backdrops in the staging repository to optimized WebP, reducing repository image storage by over 93% (from 65MB to 4.5MB).

### Fixed

- **Settings Modal Layout & ComboBox Display**: Resolved `ScrollView` type resolution in QML, fixed ComboBox blank label display for JavaScript object models, and wrapped preferences content in a self-adaptive scroll container to prevent dialog clipping.

## [0.3.3] - 2026-10-03

### Added

- **Standalone `sekiro.exe` Direct Launch**: Decoupled game launch from Steam protocol (`steam://rungameid/814380`). SMM now detects and prioritizes direct process execution (`QProcess::startDetached`) with the game root working directory for offline, standalone, or DRM-free installations.
- **Dynamic Telemetry & Manifest-Aware Stats**: Deployment metrics (linked files and disk space saved) now initialize to zero (`0` files, `0 B` saved) instead of mock defaults, accurately reflect live `.smm_manifest.json` data, and instantly zero out on environment restore.
- **HuskarUI-First Component Alignment**: Upgraded batch selection checkboxes to `HusCheckBox`, converted mod description inputs to auto-sizing `HusTextArea`, added popconfirm dialogs (`HusPopconfirm`) for mod pack preset deletion, and improved Chinese typography rendering (DemiBold 600 weight with DirectWrite NativeRendering).

### Changed

- **Standardized Metadata (`.smm_mod.json`)**: Migrated legacy `mod.json` staging metadata exclusively to `.smm_mod.json`, automatically purging old files upon normalization.
- **Archive Staging Backup (`.smm_source`)**: Preserved imported `.zip` / `.7z` archives in `.smm_source` inside each mod staging directory for lossless archive re-export.
- **Clean Export Pipeline**: Mod packs and standalone mod exports exclude internal `.smm_source` backup archives by default (`ExportOptions::include_source = false`), ensuring compact, distribution-ready output packages.
- **Custom Image Stem Normalization**: Preserved original image stem names for preview assets while automatically transcoding WebP/BMP formats to single standalone PNG files.

## [0.3.2] - 2026-10-02

### Added

- **Real-Time Preview Image Conversion & Healing**: Added native Windows Imaging Component (WIC) image normalizer in `smm_core`. Automatically converts WebP / corrupt image formats to standard PNG on upload and auto-heals legacy mod preview images. Integrated cache-busting revision tracking in QML for instant real-time UI feedback.
- **Unequip Mod Pack Preset**: Added dedicated "Unequip Mod Pack" option in the preset view to disengage active preset locks and return to freeform mod management.
- **Equipped Mod Pack Filter**: Added a toggle button in the mod management toolbar to view only mods belonging to the active mod pack, with $O(1)$ memory lookup.
- **Mod Metadata Full Editing**: Added full editing for Mod ID, display name, author, and description in `ModDrawer` with automatic safe folder renaming in staging.
- **Domain-Matched Source Icons**: Added automatic platform icon recognition for Nexus Mods, GameBanana, 3DM Mods, Bilibili, and GitHub with official brand assets.
- **Streamlined Workflow Layout**: Relocated "Save Setup as Mod Pack" directly into the Armoury view toolbar next to batch operations.
- **50% Package Size Slimming**: Pruned 20MB software rasterizer (`opengl32sw.dll`), 15MB redundant QtQuick Controls styles (FluentWinUI3, Imagine, Material, Universal, Fusion), and unreferenced embedded assets; reduced portable bundle from 135MB down to ~59MB (Release ZIP down to ~23MB).
- **QML Image Memory Downsampling (`sourceSize`)**: Constrained decoded bitmap dimensions in `ModCard` and `LaunchHubView`, avoiding full 1080p/2K image uncompressed RAM allocation and saving up to 96% per-card image memory.

## [0.3.1] - 2026-10-02

### Added

- **Batch Mod Management**: Added multi-select batch management in the Armoury view with select-all, batch enable/disable, and batch delete with confirmation modal.
- **Mod Pack Preset Import Sync**: Automatically registers imported `.smmpack` archives into `.smm_presets.json` and emits `refreshPresets()` for instantaneous preset list presentation.
- **Bilingual Batch i18n**: Completed full English translations for all batch management actions and dialogs.

### Changed

- **Reliable Desktop Update Experience**: Refactored the update system into a clean version notification and one-click installer / release page launcher, eliminating fragile in-process file overwriting, Windows process locks, and installer version desynchronization.

### Fixed

- **Settings Persistence & Mod Reload**: Resolved staging directory reverting to default on uncreated paths; added automatic recursive folder creation via `fs::create_directories` and `settings.sync()`.
- **Atomic Settings Confirmation**: Deferred language switching and directory changes to apply atomically on "Save Settings" click instead of taking immediate effect.
- **Settings Scoped ID Resolution**: Fixed `ReferenceError: stagingDirInput is not defined` by managing pending states on the root modal across independent delegates.
- **Responsive Toolbar Layout**: Restructured mod management toolbar into a two-row responsive layout, preventing right-edge truncation of view toggles and action buttons on narrow windows.
- **Mod Pack Card Visual Overhaul**: Redesigned preset cards to 156px compact cards with mod count tags, status badges, elegant fallback descriptions, and refined primary action buttons.
- **Unicode/Code Page Immunity**: Replaced ANSI code page conversions with Win32 CP_UTF8 conversions via `path_to_utf8` and `utf8_to_path`, completely resolving 'No mapping for the Unicode character exists in the target multi-byte code page' and restoring full compatibility with legacy mod repositories.
- **Path Normalization & Multi-Archive Batch Import**: Unified directory path separators to native Windows backslashes with URL percent-encoding decoding; enabled multi-file selection in `FileDialog` for batch archive importing.

## [0.3.0] - 2026-10-02

### Added

- **Architectural Rewrite in C++17 & Qt 6 / QML**: Replaced legacy Rust/Tauri stack with high-performance native C++17 libraries and Qt 6 QML desktop GUI powered by HuskarUI.
- **In-Process Core Integration**: `smm_gui` links `smm_core` directly in-process for instantaneous UI response and zero subprocess overhead.
- **Sumi-e & Modern Tactile Aesthetics**: High-contrast, restrained dark/light theme styling with custom calligraphy window icons and watermark artwork.
- **Bilingual Internationalization (i18n)**: Seamless dynamic language switching between Simplified Chinese (`zh-CN`) and English (`en-US`) with 150+ localized keys.
- **Full Environment Health Doctor**: Interactive diagnostics for `sekiro.exe`, ModEngine `dinput8.dll`, `modengine.ini`, and NTFS cross-volume link viability with automated repair.
- **Fine-Grained Asset Management**: Per-asset enable/disable toggles inside the mod drawer.
- **Immersive Card Backdrops**: Visual blurred background artwork for mod cards with automatic `.smmpack` packaging.
- **Multi-Depth Archive Parsing**: Automatic 6-level recursive nested zip extraction and loose file heuristic categorization.

### Changed

- Transitioned build system to standard CMake 3.25+ with Ninja and Visual Studio 2022 presets.
- Preserved 100% backward compatibility with v0.2.0 mod staging directories and portable `.smmpack` mod packs.
- Cleaned and modernized repository root: purged obsolete Node/Rust dependencies.

## [0.2.0] - 2026-09-22

### Added

- Community health files: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
  `SECURITY.md`, `CHANGELOG.md`, `.editorconfig`, GitHub issue forms, a pull-request template, a
  Dependabot configuration, and `rust-toolchain.toml`.
- `LICENSING.md`, which now carries the full MIT scope and third-party notices.
- CI now enforces `cargo fmt --all -- --check`, runs
  `cargo clippy --workspace --all-targets -- -D warnings`, and installs with `--frozen-lockfile`.

### Changed

- `README.md` / `README.zh-CN.md` slimmed down: the licensing analysis moved to `LICENSING.md`, and
  the architecture tree and CLI reference were corrected against the actual source.
- Rust sources normalized with `rustfmt` and 14 Clippy lints resolved; no behavior change.
- Repository hygiene: dropped the stale duplicate `apps/smm-desktop/pnpm-lock.yaml` and stopped
  tracking the generated `tsconfig.node.tsbuildinfo` and `src-tauri/gen/schemas/`.

### Fixed

- `.gitattributes` never applied its `LICENSE text eol=lf` rule — the rule had been concatenated onto
  a comment line, so license texts could be rewritten to CRLF on Windows checkouts.
- `scripts/build-installer.ps1` printed an empty path at the end of a successful packaging run; it
  referenced an undefined `$setupExe` instead of `$distSetupExe`.
- The README documented CLI flags that do not exist (`--staging-dir`, and `--game-dir` for
  `deploy` / `restore`; the real flags are `--staging` and `--target`).
- The README credited the UI to the Maple Mono typeface; SMM ships subset Inter, JetBrains Mono and
  Noto Sans SC.

## [0.1.8] - 2026-09-22

### Changed

- Licensing is consolidated: `licenses/` holds only the verbatim upstream texts, and the MIT scope
  plus third-party notices live in [LICENSING.md](LICENSING.md).

### Fixed

- Release notes are generated with PowerShell 7 (`pwsh`); Windows PowerShell 5.1 read the script
  using the ANSI codepage and mangled every non-ASCII character.

## [0.1.7] - 2026-09-22

### Added

- Declare the MIT scope explicitly and ship the third-party license texts in every release.

### Changed

- **Breaking:** ModEngine is no longer redistributed. SMM detects an installed `dinput8.dll` and
  patches `modengine.ini`, but requires the user to supply the payload themselves.

## [0.1.6] - 2026-09-20

### Added

- Self-hosted subset fonts (Inter, JetBrains Mono, Noto Sans SC) with a build-time subsetting script
  and a CI check that every artifact is present and non-empty.

### Fixed

- Restore `http-body-util` to `0.1.5` in `Cargo.lock`; an over-eager bump broke dependency
  resolution in CI.

## [0.1.5] - 2026-09-13

### Added

- Support `audio/` and `voice/` mod signatures and object prop mods.

### Fixed

- Sanitize quoted paths supplied on the command line.

## [0.1.4] - 2026-09-13

### Fixed

- Normalizer supports UI subdirectories (`hi/`, `font/`) nested under `menu/`, ignores Yabber
  unpack artifacts, and detects multi-folder mod signatures reliably.

## [0.1.3] - 2026-09-13

### Added

- Multi-file merged import, mod activation presets, and multiple mod-list view modes.
- Native file picker plus shortcuts to launch the game and open its directory.

### Fixed

- Engine diagnosis and category layout; bump the embedded ModEngine payload.

## [0.1.2] - 2026-09-13

### Added

- Unified mod source URL handling with 3DM and GameBanana recognition.

### Fixed

- Open external links in the system browser.
- Importer supports `sfx/` canonical directories, loose `texbnd`/`vfx` signatures, and recursive
  extraction of nested archives.

## [0.1.1] - 2026-09-12

### Added

- Native file picker, RAR support, ModEngine integration, and a redesigned confirmation dialog.
- Source URL provenance, `.smmpack` modpack bundling, and the `setup.exe` packaging pipeline.

## [0.1.0] - 2026-09-12

### Added

- Initial Rust workspace with `smm-core` (engine) and `smm-cli` (`smm`).
- Tauri v2 + React 18 + Tailwind CSS desktop GUI with a vector icon system.
- GitHub Actions workflows for CI and tag-triggered releases.
- MIT license, `.gitignore`, and bilingual documentation.

[Unreleased]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.6...HEAD
[0.4.6]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.5...v0.4.6
[0.4.5]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.4...v0.4.5
[0.4.4]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.3...v0.4.4
[0.4.3]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.2...v0.4.3
[0.4.2]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.3.7...v0.4.0
[0.3.7]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.3.6...v0.3.7
[0.3.6]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.3.5...v0.3.6
[0.2.0]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.8...v0.2.0
[0.1.8]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.7...v0.1.8
[0.1.7]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.6...v0.1.7
[0.1.6]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.5...v0.1.6
[0.1.5]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.4...v0.1.5
[0.1.4]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/RolinShmily/sekiro-mod-manager/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/RolinShmily/sekiro-mod-manager/releases/tag/v0.1.0
