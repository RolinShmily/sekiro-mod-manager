# 无动效

GUI 不做动效：没有页面淡入、卡片入场、悬停颜色过渡、Toast 位移。状态变化一律是即时替换。
这不是"默认关闭、可再打开"的偏好，而是实现本身——设置页里没有动效开关。

## 唯一的开关

`main.cpp` 在创建 QML 引擎之前固定关掉：

```cpp
HusTheme::instance()->setAnimationEnabled(false);
```

HuskarUI 每个组件的 `animationEnabled` 默认绑定 `HusTheme.animationEnabled`，所以这一行覆盖整棵
场景，包括组件内部的下拉展开、按压涟漪这些我们不写的动画。项目自己的 QML 里不再出现
`Behavior` / `*Animation` / `Transition`，`GuiController` 也不再读写 `reducedMotion`。

## 为什么

1. **换肤本来就该是一帧。** 主题切换会让整棵场景的颜色绑定在同一事件里全部变化；用
   `Behavior on color` 去过渡它，一次换肤就是十几个帧的全窗口重绘，观感是卡顿而不是流畅。
   以前靠 `applyThemeMode()` 临时关掉再恢复动画绕过去，现在没有可绕的东西了。
2. **这个界面不需要动效解释"东西从哪来"。** 主要操作是长任务（扫描、导入、部署），进度与结果
   由进度条、状态标签和 Toast 文案承担，位移动画不增加信息。
3. **少一层状态。** 没有"用户偏好 × 系统设置 × 当前页面"的组合，截图与视觉验收都是稳定的。

## 用静态差异表达状态

- 按压/悬停反馈直接改值：`NavItem` 按压 `scale 0.985` / `opacity 0.92`，`ModCard` 拖拽
  `scale 1.025` / `opacity 0.90`。值瞬时切换，没有过渡。
- 颜色、图标、文案三件事同时变化，不只靠颜色（见 [i18n-accessibility.md](i18n-accessibility.md)）。
- Toast 用 `opacity` 在 0 / 1 之间切换，出现与消失都是硬切。

## Toast 停留时长

动效没有，但"停留多久"仍然要定，否则错误信息还没读完就消失：

| 类型 | 停留 |
|---|---|
| `error` | 7000 ms |
| `warning` | 5500 ms |
| `info` / `success` | 3200 ms |

点击可立即关闭。Toast 是 `Accessible.AlertMessage`，读屏会朗读消息本身。

## 已知取舍

- 页面切换是硬切。对 1280×840 的桌面窗口可以接受，也避免了"切页瞬间还能看到上一页"的残影。
- HuskarUI 的 `durationFast` / `durationMid` token 仍在（组件内部用），只是当前不会被触发。

## 如果以后要加回动效

按顺序满足全部条件，并在 [../architecture/decisions.md](../architecture/decisions.md) 记一条：

1. 有单一总开关，且用户能关掉（系统"减少动画"设置优先）。
2. 位移只作用于 `transform`，不动画 `y` / `height`。
3. 动效不承担信息：关掉后状态仍然可读。
4. 不动画会整窗重绘的属性（底色、主题 token）。
