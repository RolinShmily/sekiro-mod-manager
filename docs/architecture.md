# 架构说明

本文说明 Sekiro Mod Manager 的仓库分层、核心库边界、桌面端线程模型、以及官网 Cloudflare Worker 反代与 R2 整合包分发架构。

## 1. 仓库全景与分层契约

```
src/core/      纯 C++17 静态库（smm_core），平铺头文件与实现，零 Qt 依赖、零终端输出
   ▲
   │ 直接进程内链接
src/gui/       Windows Qt 6 Quick / QML + HuskarUI 桌面客户端（SekiroModManager.exe）

worker/        Cloudflare Worker 入口：托管静态官网 + /releases/* 白名单流式反代
website/       Vite + React 19 + Tailwind CSS v4 静态产品官网（构建期 SSR 预渲染）
tests/         统一测试目录：core/ · gui/ · website/ · worker/
```

核心硬约束：

1. **核心层纯净**：`src/core/` 仅依赖标准库与 `src/core/third_party/`（`nlohmann/json`、`miniz`），通过返回值与 `smm::SmmError` 表达状态，不依赖 Qt，不写 `std::cout` / `std::cerr`。
2. **规则单一来源**：暂存扫描、目录归一化、三级冲突裁决、部署计划、NTFS 硬链接执行、`.smmpack` 读写与环境体检全部在 `smm_core` 实现；GUI 只做类型适配与流程编排。
3. **警告隔离**：根目录 `CMakeLists.txt` 定义接口目标 `smm::warnings`（MSVC `/W4 /permissive-`，GCC/Clang `-Wall -Wextra -Wpedantic`），仅挂载到自有目标；第三方子模块（HuskarUI、QWindowKit 等）由 `smm_silence_vendor_warnings()` 单独静默。
4. **跨平台核心可测**：Windows 专属 API（`CreateHardLinkW`、卷序列号、注册表探测、WIC 图片转码）隔离在 `#ifdef _WIN32` 内，POSIX 下提供等价 `stat()` / `std::filesystem` 回退，保证 `test.yml` 在 Ubuntu 与 Windows 双平台验证核心规则。

## 2. `src/core/` 模块地图（平铺布局）

外部调用方统一以 `#include "core/<header>.hpp"` 引用，`src/core/` 内部以 `#include "<header>.hpp"` 互引：

| 模块 | 文件 | 职责 |
| :--- | :--- | :--- |
| 类型与序列化 | `types.hpp/.cpp` | `ModInfo`、`AssetEntry`、`ConflictReport`、`DeployPlan`、`ModPreset`、`StagedMod` |
| 错误契约 | `error.hpp/.cpp` | `ErrorCode` 枚举、`SmmError` 异常及稳定字符串映射 |
| 目录归一化 | `normalizer.hpp/.cpp` | 识别 Nexus/社区压缩包结构，散落签名文件归位至 `parts/`、`chr/`、`menu/` 等 |
| 暂存扫描 | `loader.hpp/.cpp` | 扫描暂存区，产出 `ScanOutcome`（含失败项），自动迁移遗留 `mod.json` |
| 模组管理 | `manager.hpp/.cpp` | 启停、顺位重排、安全改名、删除、原子写入 `.smm_mod.json`、预览图 WebP 归一化 |
| 冲突仲裁 | `conflict.hpp/.cpp` | 按目标路径分组，按 `Critical / Warning / Info` 三级分级，低优先级数字胜出 |
| 部署计划与执行 | `deploy.hpp/.cpp`、`executor.hpp/.cpp` | 生成部署映射、同卷 NTFS 硬链接 / 跨卷复制、读写 `.smm_manifest.json`、纯净还原 |
| 归档与导入导出 | `extractor.hpp/.cpp`、`importer.hpp/.cpp`、`exporter.hpp/.cpp` | 内置 miniz 解压 `.zip`/`.smmpack`，外部调用 `7z`/`unrar` 处理 `.7z`/`.rar`，最多 6 层递归解包 |
| 整合包预设 | `preset.hpp/.cpp` | `.smm_presets.json` 读写与整套启停/顺位应用 |
| 环境诊断 | `doctor.hpp/.cpp` | 校验 `sekiro.exe`、`dinput8.dll`、`modengine.ini`、暂存卷一致性与部署清单 |

## 3. `src/gui/` 后端分层与线程模型

```
QML Views / Components
   │ 属性绑定 & Q_INVOKABLE
GuiController (controllers/)
   ├── Models (models/)         ModListModel / AssetListModel / PresetListModel（纯投影，不写盘）
   ├── TaskRunner (services/)   tasks_（写盘串行单线程） + scans_（只读扫描/体检单线程）
   ├── CoreService (services/)  同步核心门面 + CoreMapping (adapters/) 类型转换
   ├── UpdateService (services/) 异步更新检查：优先本站 Worker 反代，失败回退 GitHub 直连
   └── ModPreviewImageProvider (providers/) 独立图像解码线程 + QMutex 路径镜像 + QCache
```

关键并发不变量：

- **界面线程不碰磁盘**：所有写操作经 `tasks_.run()` 在单线程串行执行，防止导入/部署/改名并发破坏暂存区与 `mods/`。
- **读写双通道分离**：`scans_` 单独承接暂存扫描与环境诊断，大体积部署期间不阻塞只读状态刷新；支持 `Scan` / `Diagnose` 合并升级。
- **安全点取消**：`TaskContext::cancelled()` 以 `CancelToken` 注入 `CoreService`，在文件/路径边界抛出 `ErrorCode::Cancelled`，交由 `cancelled` 回调优雅收敛。
- **一次扫描、一次刷新**：`applyRefresh()` 在单次界面帧同步更新模组列表、冲突计数、部署计划与遥测，避免状态错位。

## 4. 官网分发、Worker 反代与 R2 整合包架构

```
用户浏览器 / SMM 桌面端检查更新
   │
   ├── https://sekiro.srprolin.com/                     → Cloudflare Workers Static Assets（website/dist）
   ├── https://sekiro.srprolin.com/releases/latest.json → Worker 反代 GitHub Release latest.json（Edge 缓存 120s），
   │                                                      校验 SemVer 与白名单 URL，重写资产链接为同域反代地址
   ├── https://sekiro.srprolin.com/releases/download/<tag>/sekiro-mod-manager-<tag>-windows-x64-(setup.exe|gui.zip)
   │                                                    → Worker 白名单流式反代 GitHub Releases 资产（Edge 缓存 24h）
   └── https://<r2-custom-domain>/<pack>.smmpack        → Cloudflare R2 公开自定义域直连（CDN + 原生 Range 断点续传，
                                                          独立于 Git 仓库与软件发布包，零 Worker 带宽开销）
```

- **桌面客户端更新回退链**：`UpdateService` 先请求 `https://sekiro.srprolin.com/releases/latest.json`，若网络异常或清单校验失败，自动回退至 `https://github.com/RolinShmily/sekiro-mod-manager/releases/latest/download/latest.json`。
- **强白名单约束**：`worker/index.ts` 拒绝任意非 GET/HEAD 请求、带查询字符串的 URL、版本与文件名不匹配的路径及非本仓库 Release 资产，防止成为开放代理。
