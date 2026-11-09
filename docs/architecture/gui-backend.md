# GUI Backend 的五种角色

```
src/gui/backend/
  controllers/GuiController.{hpp,cpp}    QML 唯一入口（上下文属性 smmBackend）
  models/                               QAbstractListModel 投影：ModListModel / AssetListModel / PresetListModel
  services/CoreService.{hpp,cpp}        无状态、同步的 smm_core 门面
  services/BackgroundTasks.{hpp,cpp}    TaskRunner / TaskContext：串行后台执行与取消
  services/UpdateFeed.{hpp,cpp}         纯逻辑：版本比较、清单解析、链接生成
  services/UpdateService.{hpp,cpp}      网络请求 + 更新对话框所需的字符串
  services/TaskTypes.hpp                CancelToken / ProgressSink 的函数签名
  adapters/CoreMapping.{hpp,cpp}        核心值类型 ↔ Qt 值类型，路径编码唯一入口
  providers/ModPreviewImageProvider      image://modpreview/<mod-id> 的解码与缓存
  GuiTypes.hpp                          两侧共用、纯数据的 Qt 投影（ModEntry、HealthInfo…）
```

## 1. controllers：只做编排

`GuiController` 是 QML 能看到的唯一 C++ 对象，在 `main.cpp` 里注册：

```cpp
engine.rootContext()->setContextProperty("smmBackend", controller);
```

它负责：

- 用户意图入口：`refreshAll`、`deploy`、`deployAndLaunch`、`restore`、`importUrls`、`saveModPack`、`deleteMods` 等；
- 流程顺序：谁的调用要先于谁，完成后要不要重新扫描或复检；
- 纯界面状态：`isBusy`、`statusText`、`healthSummary`、`canLaunch`、当前详情选中项；
- 持有两个 `TaskRunner`（`tasks_` 写、`scans_` 读）与一个 `UpdateService`；
- 设置读写（QSettings，组织 `SekiroModManager`、应用 `SMM`；Windows 默认写入注册表）。

它不负责：业务规则、结构体转换、阻塞 I/O、清单解析。

判断一个方法该不该写在控制器里的标准：**它是否只描述"界面接下来该看到什么"**。是 → 控制器；否 → `CoreService`。

## 2. models：只描述行

三个模型共享同一套契约：

- 只保存传入的行、筛选条件与展示用派生值（如顺位、格式化大小）；
- 不访问文件系统，不持久化；
- 用户动作以信号形式上报，例如 `ModListModel::enabledToggleRequested`、`prioritySwapRequested`、`reorderRequested`，`AssetListModel::assetToggleRequested`；
- 控制器的 `applyRefresh()` 一次性替换模型内容，模型内容永远来自同一次扫描。

`ModListModel` 的筛选（`filterText`、`selectedCategory`、`onlyEquippedPackMods`）在内存里完成，排序与顺位来自核心的优先级。

## 3. services：CoreService 与 TaskRunner 分工

| 类型 | 线程 | 是否抛异常 | 责任 |
|---|---|---|---|
| `CoreService` | 调用方线程（通常是后台） | 是，`SmmError` / `std::runtime_error` | 把 QString 参数翻成核心调用，把结果翻成 Qt 投影 |
| `TaskRunner` | 一个工作线程 | 否，错误转成 `failure(message)` 回调 | 串行执行、进度转发、协作取消、安全析构 |

`CoreService` 的每个方法都是 `static` 且无状态，所以从后台线程调用是安全的。它不做线程、发信号或弹窗——这些一旦混进来，就无法在无 GUI 的测试里运行（`smm_core_service_tests` 正是这样测试它）。

`UpdateFeed` 保持零 Qt 依赖，`UpdateService` 只是在外面包一层 `QNetworkAccessManager`。两者分开后，版本比较逻辑可以脱离网络单独测试（`smm_update_feed_tests`）。

## 4. adapters：一次转换，一处编码

`CoreMapping` 是唯一同时认识 `smm::` 与 Qt 类型的地方。路径转换集中在这里：

```cpp
fs::path toStdPath(const QString& qpath);
QString toQString(const fs::path& path);
```

历史上在调用点混用 `toStdWString()` 与 `toStdString()`，导致非 ASCII 暂存路径在 Windows 上断掉。任何新的跨越边界的类型都加到 `CoreMapping`，不要在模型或控制器里就地写转换。

## 5. providers：图像与线程

`ModPreviewImageProvider` 由 QML 通过 `image://modpreview/<mod-id>` 使用，在解码线程上运行。它不直接读控制器：

- 构造时接收一个 `std::function<QString()>` 闭包去问当前暂存目录；
- `GuiController` 用 `QMutex` 保护的 `previewStagingMirror_` 回答这个问题；
- 缓存是静态的，替换预览图或删除模组后调用 `clearCache()`。

这样 provider 不会与控制器/模型形成环，也不会在控制器析构后访问悬空指针——`main.cpp` 里控制器先于引擎创建、后于引擎销毁，正是为了这个顺序。

## QML 侧的约定

- QML 模块 URI 为 `SmmGui`，资源前缀 `/`，入口 `qrc:/SmmGui/qml/Main.qml`；
- 调试构建下 `--dev` 会直接从 `SMM_SOURCE_DIR/qml/Main.qml` 加载并监听目录变化，实现保存即重载；
- 任何需要写盘的按钮最终都要落到 `smmBackend` 的某个 `Q_INVOKABLE`，QML 不持有磁盘路径；
- 视图通过 `Loader` 懒加载，首次进入才实例化。

组件清单与用法见 [../design/components.md](../design/components.md)。
