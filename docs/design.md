# 设计规范

本文描述 Sekiro Mod Manager 桌面端与官网的视觉原则、组件骨架、交互约定与截图生成方式。

## 1. 桌面端视觉与布局骨架

- **视觉基调**：深色为默认，以暖金强调色（`src/gui/main.cpp` 安装 `installThemePrimaryColorBase("#c29f5d")`）配合克制的高对比中性层级，不在视图中硬编码十六进制颜色。
- **表面层次**：长页面内容直接落在页面背景上，靠图标、标题、细分隔线与留白建立分组，减少无意义的嵌套卡片与投影；仅真正浮起的层（抽屉、弹窗、Toast、忙碌徽标）使用阴影。
- **主窗口结构（`src/gui/qml/Main.qml`）**：
  - `HusCaptionBar`（40 px）：应用标识、主题切换、系统窗口控制。
  - 左侧导航导轨（208 px）：品牌头、6 个导航页（`LaunchHubView` / `ArmouryView` / `ModPackView` / `DiagnosticsView` / `SettingsView` / `AboutView`）、底部运行状态卡（点击直达诊断页）。
  - 内容舞台：`StackLayout` + `Loader` 懒加载，切页直接硬切，无冗余过渡。
  - 全局浮层：`ModDrawer`（右侧模组详情与资产级开关抽屉）、`SavePackModal`（保存整合包预设）、顶部 Toast 通知。

## 2. 动效与性能约束

- **默认静态界面**：启动阶段固定 `HusTheme.animationEnabled = false`，禁用换肤、弹窗、波纹与切页过渡动画，避免空闲帧率空转与主题切换卡顿。
- **唯一受控动效**：模组卡片与详情抽屉的预览图加载支持模糊低分辨率占位 → 清晰图 260 ms（`Easing.OutCubic`）交叉淡入，仅操作 `opacity` 属性；用户可在「设置 → 外观与语言 → 界面动效」关闭（写入 `QSettings` 的 `reducedMotion`），关闭后时长立即归零。
- **图像解码约束**：`ModPreviewImageProvider` 通过 `sourceSize` 按需下采样解码（卡片 480×270、抽屉 960×540）并做 LRU 缓存，避免原图常驻显存。

## 3. 设置页、无障碍与双语本地化

- **双列自适应表单（`SettingRow.qml`）**：左侧标签与说明上限 320 px（下限 200 px），右侧控件对齐同一起始竖线并随窗口宽度伸展（300–900 px）；长页面采用 `contentWidth: availableWidth` 铺满窗口，不留大片空白。
- **权限与启动体验**：桌面端在清单中声明 `requireAdministrator`，启动时由系统统一弹一次 UAC，根除 Steam `PreferSystem32` 策略导致的 `dinput8.dll` 加载失效问题；安装程序完成页以 `shellexec` 启动以避免错误 740。
- **双语支持**：界面源码默认简体中文，英文翻译维护于 `src/gui/translations/smm_en_US.ts`（构建期经 `lrelease` 编译嵌入 `:/i18n/smm_en_US.qm`），支持运行期实时切换。

## 4. 官网定位与设计原则

- **定位**：现代、克制、信息清晰的产品展示与下载中心（对标简洁开发者工具站），不再使用厚重水墨底图或低可读性装饰排版。
- **核心板块**：顶部清爽导航（含中英切换与 GitHub 入口）→ 清晰 Hero 与主下载 CTA → 带页签切换的桌面端实机截图预览器 → 核心能力卡片 → 三步工作流 → 软件下载中心（镜像加速 / GitHub 直连切换、Setup 安装包与便携 ZIP、SHA-256 一键复制）→ 社区 `.smmpack` 尝鲜整合包展示区（附清晰第三方版权与非官方声明）→ FAQ → 页脚。

## 5. 实机截图维护

仓库根目录 `assets/preview.webp` 与 `assets/screenshots/*.webp` 均基于虚构演示库拍摄，不涉及个人真实游戏目录：

```powershell
# 1. 生成虚构演示暂存区与部署清单（纯 PowerShell，无需外部工具）
pwsh scripts/local/seed_demo.ps1

# 2. 交互式桌面会话下批量截屏到 assets/screenshots/
pwsh scripts/local/capture_screenshots.ps1 -Theme dark
pwsh scripts/local/capture_screenshots.ps1 -Theme light
```
