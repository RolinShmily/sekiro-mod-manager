# 国际化与无障碍

## 语言模型

- **源语言是简体中文**：QML 里直接写 `qsTr("只狼模组管理器")`，中文就是 `src` 语言。
- **英文在 `.ts` 里**：`src/gui/translations/smm_en_US.ts`，共 336 条消息，全部已翻译（无 `type="unfinished"`）。
- **编译与嵌入**：`src/gui/CMakeLists.txt` 用 `Qt6::lrelease` 生成 `smm_en_US.qm`，再通过 `qt_add_resources(... PREFIX "/i18n")` 嵌进二进制，运行时加载 `:/i18n/smm_en_US.qm`。
- 切换语言不需要重启：`GuiController::language` 变化 → `main.cpp` 的 `applyLanguage()` 移除旧 `QTranslator`、装载新语言、调用 `engine.retranslate()`。
- 选择持久化在 QSettings 的 `language` 键（默认 `zh-CN`）。

新增一条文案的流程：

1. QML 里写 `qsTr("中文原文")`；
2. 用 `lupdate` 更新 `smm_en_US.ts`（不是手写 XML 结构）；
3. 填英文翻译；
4. 构建时 `lrelease` 自动重新编译。

英文长度通常比中文长 30%–50%，布局要能放下：标签用 `elide: Text.ElideRight`，说明用 `wrapMode: Text.WordWrap`，不要给固定 `width` 之后又假设文字一定放得下。

## 已知的翻译缺口

1. **核心诊断文案是英文**。`src/core/src/doctor.cpp` 里的 `DiagnosticItem::title` / `detail` / `remediation` 是硬编码英文，因为 core 不链接 Qt、不能用 `tr()`。结果是在中文界面里混着 `Game Directory`、`modengine.ini` 这类英文行。
   - 短期：这些内容本身多为路径与键名，混排可接受。
   - 长期：core 输出稳定消息键，由界面翻译。改之前先看 [../architecture/decisions.md](../architecture/decisions.md) 的遗留问题。
2. **CLI 没有本地化**。`src/cli` 只有英文，且面向脚本与 CI，这是有意的。
3. **分类名不翻译也不该翻译**。`weapon_skin`、`ui` 是元数据取值，界面用 `smmBackend.categoryLabel()` 统一映射成显示名，不要在各视图里各写一份映射。

## 键盘可达

| 快捷键 | 行为 | 位置 |
|---|---|---|
| `Ctrl+1` | 启动与工作台 | `Main.qml` |
| `Ctrl+2` | 模组管理 | `Main.qml` |
| `Ctrl+3` | 整合包预设 | `Main.qml` |
| `Ctrl+4` / `Ctrl+,` | 全局配置 | `Main.qml` |
| `Ctrl+F` | 跳到模组管理并聚焦搜索框 | `Main.qml` → `ArmouryView.focusSearch()` |
| `Ctrl+R` | 重新体检（忙碌时忽略） | `Main.qml` |
| `Space` / `Enter` | 激活当前导航项 | `NavItem.qml` |

`Ctrl+F` 的实现有一个务实的细节：目标视图可能是懒加载的，`focusSearch()` 会失败。因此用 60 ms 间隔的定时器重试最多 8 次，而不是静默失败或强行同步加载视图。

焦点可见性：

- `NavItem` 在 `activeFocus` 时绘制 2px 主色焦点环（外层 `Rectangle` 的 `border.width`）。
- 其他控件沿用 HuskarUI 自带的焦点样式，项目不覆盖。

## 读屏与语义

现有 `Accessible` 标注：

| 元素 | role | name / description |
|---|---|---|
| `NavItem` | `Accessible.Button` | 导航项文字 |
| `ModCard` | `Accessible.Grouping` | 模组名；description 为启用状态与顺位 |
| Toast | `Accessible.AlertMessage` | 消息正文 |
| 设置页路径字段 | 默认 | `只狼游戏安装根目录` / `模组暂存区目录` |
| 设置页字体搜索框 | 默认 | `搜索已安装字体` |

新增自定义可交互组件时，至少给出 `Accessible.role` 与 `Accessible.name`；纯装饰元素（图标瓦片、分隔线、背景图）保持默认，不要加无意义的 role。

## 对比度与字号

- 正文最小 12px（`font.pixelSize: 12`），且 12px 只用于元数据与徽章；说明性文字用 13px。
- 文本颜色只用语义 token（`colorTextPrimary` → `colorTextQuaternary`）。不要把 `colorTextQuaternary` 用在正文上：它在深色主题下对比度不足。
- 状态不能只靠颜色区分：诊断项同时有状态图标与文字；冲突严重度同时有文字标签。色盲用户读得出来。
- 深色/浅色都由 `HusTheme.isDark` 驱动，不在视图里维护两套色表（见 [tokens.md](tokens.md)）。

## 检查清单

提交界面前过一遍：

- [ ] 所有用户可见字符串都在 `qsTr()` 里（`#`、`v` 这类纯符号前缀除外）。
- [ ] 英文比中文长时不会截断关键信息（用 `elide` 而不是硬裁）。
- [ ] 新快捷键没有与 `Main.qml` 里已有序列冲突。
- [ ] 新组件有 `Accessible.role` / `Accessible.name`，或明确是装饰元素。
- [ ] 关闭"减少动效"后界面仍然完整可用。
