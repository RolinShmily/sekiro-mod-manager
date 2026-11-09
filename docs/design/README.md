# 设计分册

本分册约束"界面长什么样、怎么动、怎么被读屏与键盘使用"。它描述**现有实现**，不是愿景稿：文档里的每个数值都能在 `src/gui/qml` 里找到出处。

## 视觉基调

- 一只狼的模组装配工坊：深色为默认，纸墨与金的点缀，不用大面积高饱和色。
- 主色基座在 `src/gui/main.cpp` 安装为 `#c29f5d`（`installThemePrimaryColorBase`），所有强调色由它派生。
- 内容直接落在页面背景上，分组靠图标、标题、细分隔线与留白；不给分组套卡片表面，也不给分组加投影。
- 只有真正浮起的元素（Toast、忙碌徽标、抽屉、弹窗）才有阴影。
- 颜色、圆角、动效时长一律来自 `HusTheme.Primary.*`，不在视图里写死十六进制值。

## 页面骨架

```
HusWindow (Main.qml)
├── HusCaptionBar            40px：品牌标识 + 主题切换 + 系统按钮
├── 左侧导航导轨             208px：Logo 区、5 个 NavItem、底部运行状态卡
├── 内容舞台                 StackLayout，5 页，切换时 180ms 淡入
├── ModDrawer               右侧详情抽屉（全局单例）
├── DoctorModal             体检弹窗
├── SavePackModal           保存整合包弹窗
└── Toast 浮层              顶部居中，按严重程度 3.2s / 5.5s / 7s
```

五个页面：启动与工作台（`LaunchHubView`）、模组管理（`ArmouryView`）、整合包预设（`ModPackView`）、全局配置（`SettingsView`）、关于（`AboutView`）。

## 分册目录

- [screenshots.md](screenshots.md) — 五个页面的实拍，以及每屏在证明什么
- [tokens.md](tokens.md) — 颜色、字号、圆角、间距、动效时长的取值与语义
- [components.md](components.md) — PageHeader / SectionCard / ModCard / ModDrawer / NavItem 等复用组件
- [motion.md](motion.md) — 动效预算、缓动、减少动效
- [i18n-accessibility.md](i18n-accessibility.md) — 中英双语、键盘可达、对比度、焦点

## 判断一条 UI 改动是否合格

1. 只用 `HusTheme.Primary.*` 与 `HusIcon.*`，没有裸色值与 emoji 图标。
2. 可点击区域不小于 32×32，键盘能到达并看见焦点环。
3. 文案走 `qsTr()`，中英文都能放下（英文通常更长）。
4. 动效都有 `enabled: HusTheme.animationEnabled` 守卫。
5. 空态、加载态、错误态都有对应文案（参考 `ArmouryView` 的 `isFiltering` / `firstScanPending` 区分）。
