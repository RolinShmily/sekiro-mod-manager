# 动效

动效的职责只有两个：告诉用户"状态变了"，以及告诉用户"东西从哪来、到哪去"。不做装饰性循环动画。

## 总开关

一切动效都必须能被关掉：

```qml
Behavior on color {
    enabled: HusTheme.animationEnabled
    ColorAnimation { duration: HusTheme.Primary.durationMid }
}
```

`HusTheme.animationEnabled` 由设置的"减少动效"驱动：`GuiController::setReducedMotion()` → `HusTheme::instance()->setAnimationEnabled(!reducedMotion)`。启动时从 QSettings 读取，因此关闭后重启仍然关闭。

`Main.qml` 还监听了 `HusTheme.onAnimationEnabledChanged`，在关闭动效的瞬间停止页面淡入并把 `opacity` 复位为 1——否则正在播放的动画会停在中间帧。

## 主题切换不是动画

切换主题会让整棵场景的每个颜色绑定在同一事件里全部变化。如果让 `Behavior on color` 去"过渡"它，一次换肤就变成十几个帧的全窗口重绘，实际观感是卡顿而不是流畅。因此 `GuiController::applyThemeMode()` 在 `setDarkMode()` 前后临时关掉 `animationEnabled`，下一轮事件循环再恢复：换肤一帧完成，之后所有动效照旧。

同理，主题相关的图片不在切换时重新解码：首屏的水墨背景与两个书法水印都常驻（按显示尺寸 `sourceSize` 解码 + `asynchronous: true`），换肤只改 `opacity`。

## 现有节奏

| 场景 | 时长 | 缓动 | 位置 |
|---|---|---|---|
| 导航项按压 | 80 ms | `OutQuad` | `NavItem.qml` |
| 导航项底色 | `durationFast`（100 ms） | 默认 | `NavItem.qml` |
| 页面切换淡入 | 180 ms | `OutQuad` | `Main.qml` |
| 窗口底色 | 180 ms | `OutQuad` | `Main.qml` |
| Toast 位移与淡入 | 180 / 160 ms | `OutCubic` / `OutQuad` | `Main.qml` |
| 卡片底色 | `durationMid`（200 ms） | 默认 | `SectionCard.qml` |
| 卡片入场 | 200 ms 淡入 + 220 ms 上浮 10px | `OutCubic` | `SectionCard.qml` |

经验区间：按压反馈 80–120 ms，状态过渡 150–200 ms，入场 200–250 ms。超过 300 ms 的过渡会让人觉得界面"黏"。

## 三条硬规则

1. **动效不能改变布局。** 位移动画一律作用于 `transform: Translate` 或 `scale`，不要动画 `y` / `height`，否则每一帧都会触发重排。`SectionCard` 的入场就是 `Translate { id: slide }` + `NumberAnimation { target: slide; property: "y" }`。
2. **动效不能被误触发。** `Behavior` 只加在真正会变化的属性上；把 `Behavior on x` 加在由布局驱动的属性上，会和布局互相拉扯。
3. **动效不承担信息。** 关闭动效后，用户仍能看出状态（颜色、图标、文案都在），动画只是让变化更平滑。

## 页面切换

`StackLayout` 切页时播放一次透明度动画：

```qml
function animatePage() {
    viewAnim.stop();
    opacity = 1.0;
    if (HusTheme.animationEnabled) viewAnim.restart();
}
```

`Loader` 懒加载的页面在 `onLoaded` 里再调一次 `animatePage()`，避免"第一次进入没有入场动画"。异步加载完成时若已是当前页才播放，否则切到该页时动画会提前消耗掉。

## 入场错落

同页多个 `SectionCard` 用 `entranceIndex` 控制延迟：`PauseAnimation { duration: entranceIndex * 45 }`。45 ms 的间隔足以读出顺序，又不会让最后一张卡片等太久。超过 6 个分组时不要再递增，改为全部同时入场。

## Toast

按严重程度给阅读时间：

| 类型 | 停留 |
|---|---|
| `error` | 7000 ms |
| `warning` | 5500 ms |
| `info` / `success` | 3200 ms |

点击可立即关闭。Toast 是 `Accessible.AlertMessage`，读屏会朗读消息本身。

## 已知不一致

- `NavItem` 的按压缩放用字面量 `80`，而同文件的状态色用 `HusTheme.Primary.durationFast`（100），两者对不齐。新增代码优先用 token；要统一时以 token 为准。
- 窗口淡入 180 ms 与 HuskarUI 的 `durationMid`（200）不同，是刻意压短，避免整窗切换显得拖沓。
