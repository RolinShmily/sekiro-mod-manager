# 设计 token

所有 token 来自 HuskarUI 的 `HusTheme` 单例（`src/gui/third_party/HuskarUI`），项目只覆盖主色基座与字体。**视图里不允许出现裸色值**，深色/浅色差异用 `HusTheme.isDark` 的三元表达式，而不是两套硬编码颜色表。

## 颜色

| Token | 语义 | 典型用法 |
|---|---|---|
| `HusTheme.Primary.colorBgBase` | 窗口底色 | `HusWindow.color` |
| `HusTheme.Primary.colorFillQuaternary` | 最低对比填充 | 导航导轨背景 |
| `HusTheme.Primary.colorFillTertiary` | 三级填充 | 图标瓦片底、卡片内分组 |
| `HusTheme.Primary.colorFillSecondary` | 二级填充 | 状态卡背景 |
| `HusTheme.Primary.colorBorderSecondary` | 次级边框 | 卡片描边、状态卡描边 |
| `HusTheme.Primary.colorSplit` | 分割线 | 导航右缘 1px 竖线、`HusDivider` |
| `HusTheme.Primary.colorTextPrimary` | 主文本 | 页面标题、卡片标题、数值 |
| `HusTheme.Primary.colorTextSecondary` | 次文本 | 正文、导航未选中项 |
| `HusTheme.Primary.colorTextTertiary` | 辅助文本 | 副标题、说明、元数据 |
| `HusTheme.Primary.colorTextQuaternary` | 最弱文本 | 分组小标题（"导航"） |
| `HusTheme.Primary.colorTextDisabled` | 禁用文本 | 不可用控件 |
| `HusTheme.Primary.colorPrimary` | 主色 | 选中导航、图标、"诊断 ↗"链接 |
| `HusTheme.Primary.colorPrimaryBorder` | 主色描边 | 忙碌徽标边框、提示浮层边框 |
| `HusTheme.Primary.colorSuccess` | 成功 | 诊断通过、已启用状态点 |
| `HusTheme.Primary.colorWarning` | 警告 | 有异常但不阻断、硬链接回退 |
| `HusTheme.Primary.colorError` | 错误 | 失败通知、危险操作 |
| `HusTheme.Primary.colorInfo` | 信息 | 中性提示 |

主色基座：`src/gui/main.cpp` 里 `HusTheme::instance()->installThemePrimaryColorBase(QColor("#c29f5d"))`。改这一个值就能整体换肤，不存在"某处忘了改"。

交互态的叠层用白色/黑色透明度，而不是另取颜色：

```qml
color: {
    if (navItem.active) return HusTheme.isDark ? Qt.rgba(1,1,1,0.09) : Qt.rgba(0,0,0,0.06);
    if (navHover.containsMouse) return HusTheme.isDark ? Qt.rgba(1,1,1,0.05) : Qt.rgba(0,0,0,0.04);
    return "transparent";
}
```

这样换主题时叠层自动跟随，也不会在深色下出现"刺眼白光"。

## 字体

- 字体族由 `GuiController::defaultFontFamily()` 决定，优先级：`Noto Sans SC` → `Source Han Sans SC` → `Microsoft YaHei UI` → `Segoe UI`。
- 用户在设置页选择的字体会持久化到 QSettings，并在启动时通过 `applyFontFamily()` 应用。
- 字号是固定档位（`font.pixelSize`），没有全局比例函数：

| 字号 | 用途 |
|---|---|
| 20 | 页面标题（`PageHeader.title`） |
| 15 | 卡片标题、导航项、次级标题 |
| 14 | 正文、模组名称 |
| 13 | 说明、副标题、状态文本 |
| 12 | 元数据、徽章、辅助注释 |

不要引入 11 或 10：在 96 DPI 下中文字形会糊。

## 圆角

`radiusPrimary*` 由 HuskarUI `Index.json` 的 `radiusBase` 派生（`radiusPrimary` = base−1，`radiusPrimaryLG` = base−2，默认 `radiusBase` 为 6）。

- 卡片、图标瓦片、状态卡：`radiusPrimaryLG` 或 `radiusPrimary`。
- 导航项、按钮：`radiusPrimary`。
- 胶囊（Toast、忙碌徽标）：直接写 `height / 2`。

## 间距

没有 spacing token，沿用固定的 4 的倍数：

| 值 | 用途 |
|---|---|
| 2–3 | 标题与副标题之间、图标与文字之间 |
| 4 | 导航项之间 |
| 8 | 行内元素、徽章与文本 |
| 12 | 导航内边距、列表项内边距 |
| 14 | 页面内区块之间（`ColumnLayout.spacing`） |
| 20 | 内容舞台外边距、卡片内边距 |

## 动效时长

GUI 不做动效（见 [motion.md](motion.md)），所以时长 token 在当前代码里没有使用者：
`HusTheme.Primary.durationFast`（100 ms）、`durationMid`（200）、`durationSlow`（300）只在 HuskarUI
组件内部生效。新增代码不要引入动画；真需要时先读 [motion.md](motion.md) 的四个前提。

## 层级与阴影

内容分组**不靠阴影**：设置页与关于页的分组直接落在页面背景上，用图标、标题、`HusDivider`
和留白分层。给每组再套一层带投影的卡片，会在背景上又出现一个矩形，视觉上像页面里另开了一块区域。

阴影只用于真正"浮在内容之上"的元素：Toast 浮层、右下角忙碌徽标、抽屉与弹窗。它们用
`HusShadow` / 主题自带的 popup 投影，取值由 HuskarUI 决定，不在视图里手写。
