# 复用组件

组件位于 `src/gui/qml/components/`，由 `src/gui/CMakeLists.txt` 的 `qt_add_qml_module(... QML_FILES ...)` 登记。新增组件必须同时加进该列表，否则运行时找不到类型。

## PageHeader — 页面标题区

```qml
PageHeader {
    iconSource: HusIcon.HomeOutlined
    title: qsTr("启动与工作台")
    subtitle: qsTr("……")
    extra: HusButton { text: qsTr("操作") }
}
```

- 结构：44×44 图标瓦片（`colorFillSecondary` 底 + 主色图标）+ 标题（20px 粗体）+ 副标题（13px `colorTextTertiary`）+ 右侧 `extra` 插槽。
- 图标瓦片存在的理由：分页后仅靠字号区分层级不够，形状与图标让人先认出"这是哪一页"。
- `extra` 是 `Component`，不是默认属性；传 `null` 时 `Loader` 不激活。

## SectionCard — 分组卡片

```qml
SectionCard {
    iconSource: HusIcon.SettingOutlined
    title: qsTr("游戏与暂存区")
    description: qsTr("……")
    entranceIndex: 0        // 同页内按 45ms 递增形成错落
    ColumnLayout { /* 直接写子项 */ }
}
```

- 默认属性别名是 `bodyData`（内部 `ColumnLayout.data`）。**不能叫 `contentData`**：`Control` 已将其声明为 FINAL，QML 会拒绝覆盖。
- 自带：行内图标（直接画在页面背景上，没有图块底）、标题/说明、`HusDivider`、入场动画。
- **不画卡片表面**：分组内容直接落在页面背景上。早期版本给分组套了一层 `HusFrame` + 投影，
  在背景上又出现一个矩形，看起来像"页面里另开一块 DOM"，因此已移除。
- 用于设置页与关于页等"长表单分组"；列表页不要用整块面板包裹每一行。

## ModCard — 模组卡片

```qml
ModCard {
    modId, modName, modVersion, modAuthor, modCategory, modDesc, modSourceUrl
    modPriority, modEnabled, modRank, rankCount, rankModel
    previewImagePath, showBackdrop, compact, batchMode, isSelected
}
```

信号：`toggleActive(bool)`、`requestDetails()`、`prioritySelected(int)`、`dragEnded(real, real)`、`toggleSelected()`。

- 两种形态：默认卡片（可带模糊背景图，`showBackdrop`）与 `compact` 列表行；`batchMode` 下显示复选框。
- 优先级以"顺位"呈现：`modRank` 由控制器算好，卡片不自己排序。
- 无障碍：`Accessible.role: Accessible.Grouping`，`Accessible.name` 为模组名，`Accessible.description` 说明启用状态与顺位。
- 卡片不写盘：启停、改顺位、拖拽都只是发信号，最终由 `ArmouryView` 转给 `smmBackend`。

## ModDrawer — 右侧详情抽屉

`HusDrawer` 子类，全局单例（在 `Main.qml` 中声明一次）。内容分三段：

1. 预览背景图 + `更换图片` / `随 .smmpack 打包`；
2. `模组信息与元数据`：ID、名称、作者、描述、分类、来源链接，`保存修改` 一次性提交（控制器负责改名）；
3. 底部动作：`打开目录`、`导出 (.zip)`、`删除模组`（删除走确认弹窗）。

抽屉只绑定 `smmBackend.currentMod*` 属性与 `assetListModel`，不持有自己的数据副本；`syncInputs()` 在打开或切换模组时把属性回填到输入框。

## NavItem — 导航项

`label`、`iconSource`、`active`，信号 `clicked()`。

- 固定 42px 高：被布局压缩会让命中区与绘制区分离。
- 激活态 = 半透明叠层 + 左侧 3×16 主色指示条 + 主色图标与文字。
- 键盘可达：`Keys.onSpacePressed` / `onReturnPressed` / `onEnterPressed` 都触发点击，`activeFocus` 时显示 2px 主色焦点环。
- `Accessible.role: Accessible.Button`。

## SavePackModal

- `SavePackModal`：`packName`、`packDesc`、`packNameEn`、`packDescEn` 四个属性，`submitSave()` 调 `smmBackend.saveModPack(...)`；中英双语字段对应 `.smm_presets.json` 里的 `name_en` / `description_en`。

## DiagnosticsView — 环境检测页

- 只读展示 `healthItems`（五项），按 `status` 给图标与标签上色，异常项带出 `remediation`。
- 两个动作都在页头：「检测环境」重跑体检，「一键装配 / 修复 ModEngine」调 `smmBackend.setupEngine()`；修复不会在检测过程中自动发生。
- 以前这是 `DoctorModal` 弹窗，改成分页后诊断不再打断操作，结果也能停留、可回看。

## 组合规则

1. 页面 = `PageHeader` + 若干个 `SectionCard` 或自有布局；不要在页面里重复实现标题区。
2. 浮层一律挂在 `Main.qml`（抽屉、弹窗、Toast 各一个实例），页面只负责 `open()`。
3. 组件不认识 `smmBackend` 之外的对象；需要新数据时在控制器上加属性，而不是让组件直接读模型以外的东西。
4. 空的 `Component` 插槽不要传 `Item {}` 占位，传 `null` 让 `Loader` 完全不创建。
