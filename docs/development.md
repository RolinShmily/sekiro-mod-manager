# 开发与发布指南

本文面向贡献者与维护者，覆盖本地编译、测试套件、Windows 打包、官网与 Worker 开发及 GitHub Actions 流水线。

## 1. 本地编译（C++17 / Qt 6）

### 环境要求

- Windows 10/11 x64、Visual Studio 2022（MSVC v143）、CMake 3.21+、Ninja
- Qt 6.8+（桌面端完整构建需要 `Core / Gui / Quick / QuickControls2 / Network / LinguistTools`）
- Python 3.9+（配置阶段自动执行 `scripts/patch_huskarui.py`）
- 仅编译核心库与测试时（Windows / Linux 均可），无需安装 Qt

### 常用命令

```bash
# 克隆（含 HuskarUI 子模块）
git clone --recurse-submodules https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager

# 完整桌面端（Debug / Release）
cmake --preset msvc-x64-debug -DSMM_BUILD_TESTS=ON
cmake --build --preset build-debug

cmake --preset msvc-x64-release -DSMM_BUILD_TESTS=ON
cmake --build --preset build-release

# 仅核心库与核心测试（无 Qt 环境）
cmake --preset core-only
cmake --build --preset build-core
```

若本机 Qt 路径与 `CMakePresets.json` 中的默认路径不同，可在命令行传 `-DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2022_64"` 覆盖。

## 2. 统一测试套件（`tests/`）

所有测试统一收口在顶层 `tests/` 目录：

| 目录 | 覆盖范围 | 运行方式 |
| :--- | :--- | :--- |
| `tests/core/` | 业务规则（`core_business_tests.cpp`）、环境体检（`doctor_tests.cpp`） | `ctest --test-dir build/<preset> --output-on-failure` |
| `tests/gui/` | `UpdateFeed` 清单与镜像解析（`update_feed_tests.cpp`）、无界面 `CoreService` 适配（`core_service_tests.cpp`） | `ctest --test-dir build/<preset> --output-on-failure` |
| `tests/worker/` | Cloudflare Worker `/releases/*` 反代、清单重写与白名单安全校验（`worker.test.mjs`） | `node --experimental-strip-types --test tests/worker/*.test.mjs` |
| `tests/website/` | 官网 Release 清单解析、镜像路由与 `.smmpack` 清单解析（`downloads.test.mjs`） | `node --experimental-strip-types --test tests/website/*.test.mjs` |

## 3. 官网与 Cloudflare Worker 开发

仓库根目录是 pnpm workspace（Node.js 22+、pnpm 11）：

```bash
pnpm install --frozen-lockfile --ignore-scripts
pnpm dev        # 启动 Vite 本地开发服务器（127.0.0.1:5173）
pnpm test       # 运行 tests/worker 与 tests/website 全部单测
pnpm lint       # TypeScript 类型检查（website + worker）
pnpm build      # 汇总生产依赖许可证 + Vite 客户端/SSR 构建 + 首页预渲染
pnpm preview    # 本地预览构建产物（127.0.0.1:4173）
```

## 4. Windows 打包脚本

`scripts/package-windows.ps1` 与 CI 发布流水线共用同一套打包逻辑，产出：

- `sekiro-mod-manager-vX.Y.Z-windows-x64-gui.zip`（裁剪冗余 Qt 样式与工具链后的便携包，随附 `LICENSE.txt`、`THIRD_PARTY_NOTICES.txt`）
- `sekiro-mod-manager-vX.Y.Z-windows-x64-setup.exe`（基于 `installer/installer.iss` 的 Inno Setup 安装程序）

```powershell
pwsh scripts/package-windows.ps1 -BuildDir build/msvc-x64-release -OutDir dist -Version v0.4.8
```

## 5. GitHub Actions 工作流

| 工作流 | 触发方式 | 职责 |
| :--- | :--- | :--- |
| `.github/workflows/test.yml` | Push / PR / 手动 | `core`（Ubuntu + Windows 矩阵）、`gui`（Windows Qt 6 + CTest）、`worker-and-website`（Node 单测 + Typecheck） |
| `.github/workflows/build.yml` | 被 `release.yml` 调用 / 手动 | 构建 Windows Release，跑测试，打包 `setup.exe` 与便携 `gui.zip`，生成 `latest.json` 与 `release-notes.md` 并上传 `release-bundle` |
| `.github/workflows/release.yml` | Tag `vX.Y.Z` / 手动 | 校验 Tag、CMake `SMM_APP_VERSION` 与 `CHANGELOG.md` 一致性 → 调用 `build.yml` → 发布 GitHub Release |
| `.github/workflows/deploy.yml` | Push `main`（`website/**`、`worker/**` 等） / 手动 | 跑 Worker/Website 测试与类型检查 → 构建静态站 → `wrangler deploy` 部署至 Cloudflare Worker |

### 发版步骤

1. 更新 `CMakeLists.txt` 中的 `SMM_APP_VERSION`、`installer/installer.iss` 默认版本号以及 `CHANGELOG.md`（添加 `## [X.Y.Z]` 段落）。
2. 提交并推送到 `main`，确认 `test.yml` 全绿。
3. 创建并推送标签 `git tag vX.Y.Z && git push origin vX.Y.Z`，`release.yml` 将自动调用 `build.yml` 构建并发布 Release，随后 Cloudflare Worker 反代在 120 秒缓存过期后自动对外生效最新版本。

## 6. 许可与合规提醒

- 本项目自有代码采用 MIT（`LICENSE`）；第三方许可汇总与完整上游文本统一维护在根目录 `THIRD_PARTY_NOTICES`。
- 禁止向 Git 仓库提交 `dinput8.dll` 或任何第三方社区 Mod 资产；社区 `.smmpack` 尝鲜包仅通过外部 Cloudflare R2 独立托管，并在 `website/public/packs.json` 中登记元数据。
