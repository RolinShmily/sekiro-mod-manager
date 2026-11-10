# 异步与线程

## 一句话规则

**界面线程只碰 Qt 对象，磁盘只在 `TaskRunner` 的工作线程上动，结果通过队列回到界面线程。**

## TaskRunner 的契约

`src/gui/backend/services/BackgroundTasks.{hpp,cpp}`

- 线程池大小固定为 1（`pool_.setMaxThreadCount(1)`），同一实例上**同时只有一个任务**。
- `run()` 在忙碌时返回 `false`，**不排队**。调用方（`GuiController`）决定要不要记下来稍后重试。
- 任务签名为 `Job = std::function<void(TaskContext&)>`，异常在 worker 内被捕获并转成 `failure(message)`。
- 完成回调（`success` / `failure` / `cancelled`）通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 派发，**只在界面线程执行，且最多一次**。
- 派发顺序被刻意固定：先交付结果，再清 `busy`，这样在结果处理里再排队的任务能看到"忙"状态并靠 `busyChanged` 启动，两个结果不可能乱序应用。
- 析构时先置取消标志，再 `pool_.waitForDone()`：对象销毁后不会再有 worker 用到悬空 `this`。

为什么不用 `QThreadPool::start()` 直接撒任务：暂存目录与游戏 `mods/` 是共享可变资源，两个导入/部署/改名并发会互相破坏。串行是正确性要求，不是性能取舍。

## 两个实例，权限不同

`GuiController` 持有两个 runner：

| 实例 | 承担 | 说明 |
|---|---|---|
| `tasks_` | 部署、还原、导入、导出、预览优化、元数据写入 | 会修改暂存区或游戏目录，必须串行 |
| `scans_` | 暂存扫描、环境体检、运行中进程探测 | 只读，单独一条通道，避免诊断等待部署 |

只读工作如果也挤进 `tasks_`，一次大部署期间"启动前检查"就会排在后面，用户看到的是无响应而不是"检查中"。

## 取消

- 取消标志是 `std::shared_ptr<std::atomic_bool>`，`TaskRunner` 持有，`TaskContext` 共享。
- `TaskContext::cancelled()` 在**安全点**被查询：部署的每个文件之间、导入的每个路径之间。核心不知道 Qt，也不知道取消标志，服务层把 `cancelled` 当成 `CancelToken` 传进 `CoreService`，后者在循环里调用它并抛 `SmmError(ErrorCode::Cancelled)`。
- `ErrorCode::Cancelled` 不算错误：`TaskRunner` 把它归到 `cancelled()` 回调，界面显示"已取消"而不是红色失败。

## 刷新管线：一次扫描，一次更新

`GuiController::requestRefresh(RefreshScope)` 有两个档位：

| 档位 | 做什么 | 触发场景 |
|---|---|---|
| `Scan` | 扫描暂存区 + 预设 + 冲突 + 计划 + 部署遥测 | 任意写操作成功后 |
| `Diagnose` | 上面的全部，再加一次完整环境体检 | 打开启动器、点击「启动只狼」、诊断页「检测环境」 |

合并策略：

- `scans_` 忙时把请求记为 `refreshQueued_`，待下一个 `busyChanged(false)` 时重放；
- 合并期间收到 `Diagnose` 会升级档位，`Scan` 不会把 `Diagnose` 降级；
- `refreshAll()` 等价于 `Diagnose`。

体检刻意只有三个入口：打开启动器、点击启动、在诊断页显式点击「检测环境」。写操作、失败通知、设置变更只做 `Scan`（或直接丢弃过期的体检结果见 `invalidateHealth()`），因此空闲窗口不会为了诊断去扫盘或枚举进程。

一次性应用：`applyRefresh()` 在一片代码里设置模型、遥测、体检结果，因此界面上看到的模组列表、冲突数、部署文件数永远来自同一次扫描，不会出现"列表是新扫的、计划是旧的"。

## 界面永不阻塞的检查点

新增一个会写盘的入口时，按顺序确认：

1. 它是否通过 `tasks_.run()` 执行，而不是在 `Q_INVOKABLE` 里直接调用 `CoreService`？
2. 它是否在成功后 `requestRefresh(...)`，让界面重新对齐磁盘？
3. 它是否在忙碌时用 `stagingWritable()` 之类的守卫拒绝并发写？
4. 失败时是否走 `reportFailure()`（通知 + 状态文本），而不是把异常吞掉？
5. 长循环里是否查询了 `TaskContext::cancelled()`？

## 图像解码线程

`ModPreviewImageProvider` 的 `requestImage()` 运行在 Qt 的解码线程上，不是界面线程。它对暂存目录的读取通过 `QMutex` 保护的镜像字符串完成（`previewStagingMirror_`，只在界面线程写入），因此不存在"读到一半路径被改"的撕裂。

解码结果的 `QImage` 进入静态 `QCache`；模组被删除、预览被替换或热重载时调用 `ModPreviewImageProvider::clearCache()`。

## 定时与生命周期

- 没有周期性诊断定时器：体检只在 [data-flows.md](data-flows.md) 列出的三个入口运行。
- `TaskRunner`（`tasks_` / `scans_`）与 `UpdateService` 是 `GuiController` 的值成员，随控制器一起析构，不额外挂父对象；动态创建的 `QTimer` 以控制器为父对象。两条路径都保证退出时先置取消标志、再等 worker 结束。
- `QML` 的 `Loader` 懒加载会让视图在首次进入时才创建；切页没有淡入动画（GUI 无动效），所以 `isCurrentItem` 变化不触发任何额外渲染——这些都在界面线程，与后台任务无关。
