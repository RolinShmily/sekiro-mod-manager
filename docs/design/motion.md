# 动效

GUI 只保留一种动效：**模组预览图的模糊占位 → 清晰图交叉淡入**。除此之外没有页面淡入、
卡片入场、悬停颜色过渡、Toast 位移；切页与状态变化依旧是硬切。

## 唯一的动效

模组卡片（`ModCard`）与详情抽屉（`ModDrawer`）的预览区有两层 `Image`：

1. 占位层把同一张预览按 40×23（抽屉 64×36）解码，再平滑放大——成本接近纯色块，但在清晰图
   返回前就给出画面色调与结构。
2. 清晰图 `status === Image.Ready` 后淡入，占位层同时淡出；两者都用

   ```qml
   Behavior on opacity { NumberAnimation { duration: fadeDuration; easing.type: Easing.OutCubic } }
   ```

   时长 260 ms。它不改变布局，只在 `opacity` 上做交叉淡入，所以不会引起重排或整窗重绘。

网格一次性加载多个卡片时，用户先看到一排模糊占位，再逐渐清晰，而不是一张张空白卡片"弹"
出来。

## 唯一的开关

设置 → 外观与语言 → **界面动效**（`GuiController::reducedMotion`，持久化到 `QSettings`）。
关闭后 `fadeDuration = 0`，所有过渡立即完成；状态仍然完全可读。

HuskarUI 组件自身的动画仍由 `main.cpp` 固定关闭：

```cpp
HusTheme::instance()->setAnimationEnabled(false);
```

这样主题切换依旧是一帧完成，组件内部的下拉与涟漪也不会重新动起来。

## 为什么只有这一处

1. **换肤本来就该是一帧。** 主题切换会让整棵场景的颜色绑定在同一事件里全部变化；用
   `Behavior on color` 过渡它只能得到十几帧的全窗重绘。
2. **图片加载是唯一会"跳"的地方。** 主要操作是长任务（扫描、导入、部署），进度与结果由
   进度条、状态标签和 Toast 文案承担；页面切换与卡片入场不增加信息。
3. **动效不承担信息。** 关掉后预览仍然会显示，只是没有淡入。
4. **不动画 `y` / `height` / 底色。** 只有 `opacity`，不触发布局。

## 用静态差异表达其余状态

- 按压/悬停反馈直接改值：`NavItem` 按压 `scale 0.985` / `opacity 0.92`，`ModCard` 拖拽
  `scale 1.025` / `opacity 0.90`。值瞬时切换，没有过渡。
- 颜色、图标、文案三件事同时变化，不只靠颜色（见 [i18n-accessibility.md](i18n-accessibility.md)）。
- Toast 用 `opacity` 在 0 / 1 之间切换，出现与消失都是硬切。

## Toast 停留时长

| 类型 | 停留 |
|---|---|
| `error` | 7000 ms |
| `warning` | 5500 ms |
| `info` / `success` | 3200 ms |

点击可立即关闭。Toast 是 `Accessible.AlertMessage`，读屏会朗读消息本身。

## 已知取舍

- 页面切换是硬切。对 1280×840 的桌面窗口可以接受，也避免了"切页瞬间还能看到上一页"的残影。
- HuskarUI 的 `durationFast` / `durationMid` token 仍在（组件内部用），当前不会被触发。
- 模糊占位是"低分辨率放大"的近似，不是高斯模糊：没有引入 `Qt5Compat.GraphicalEffects`，
  避免多一个运行时依赖。

## 新动效的准入条件

1. 有单一总开关（`reducedMotion`），且用户能关掉。
2. 位移只作用于 `transform`，不动画 `y` / `height`。
3. 动效不承担信息：关掉后状态仍然可读。
4. 不动画会整窗重绘的属性（底色、主题 token）。

满足全部条件后，在 [../architecture/decisions.md](../architecture/decisions.md) 记一条。
