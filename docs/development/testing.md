# 测试

## 四个套件

| CTest 名称 | 源文件 | 覆盖 |
|---|---|---|
| `smm_doctor_tests` | `src/core/tests/doctor_tests.cpp` | 五项体检、`modengine.ini` 解析与修补幂等性 |
| `smm_core_business_tests` | `src/core/tests/core_business_tests.cpp` | 目录解析、id 清洗与改名、优先级重排、部署清单、复用已扫描的体检 |
| `smm_update_feed_tests` | `src/gui/tests/update_feed_tests.cpp` | 版本比较、清单解析、多语言发布说明、链接生成（零 Qt） |
| `smm_core_service_tests` | `src/gui/tests/core_service_tests.cpp` | `CoreService` + `CoreMapping` 的无窗口适配层 |

全部是独立可执行文件，不依赖测试框架：`check(条件, "断言名")` 失败即抛异常，进程退出码非零。断言名会被 CI 原样打印，所以要写成能独立读懂的一句话（`"wrong engine target blocks even if folder exists"`）。

## 运行

测试目标默认**不构建**，需要显式打开开关：

```bash
cmake --preset msvc-x64-debug -DSMM_BUILD_TESTS=ON
cmake --build --preset build-debug
ctest --test-dir build/msvc-x64-debug --output-on-failure
```

只跑一个：

```bash
ctest --test-dir build/msvc-x64-debug -R smm_core_business_tests --output-on-failure
```

直接执行二进制（输出更完整，每个 PASS 一行）：

```bash
./build/msvc-x64-debug/smm_core_business_tests.exe
```

`core-only` 预设下 `ctest` 只有前两个套件；GUI 层测试需要 Qt。

## 测试的边界

- **无 Qt 依赖的套件必须保持无 Qt。** `UpdateFeed` 之所以从 `UpdateService` 里拆出来，就是为了让版本比较能在没有 `QNetworkAccessManager`、没有窗口、没有事件循环的情况下测。
- **不测真实游戏目录。** 所有用例都在临时目录里构造夹具（创建 `mod.json`、伪造 `dinput8.dll` 大小、写 `modengine.ini`），跑完即弃。
- **不测视觉效果。** 视觉只能人工看截图，见下文。
- **不启动 GUI 进程。** `smm_core_service_tests` 只链接 `Qt6::Core`。

## `smm_core_service_tests` 的 Windows 细节

该测试链接 `Qt6::Core`，而构建树没有部署 Qt DLL。`src/gui/CMakeLists.txt` 用 `cmake -E env` 把 Qt 的 `bin` 目录临时前置到 `PATH`：

```cmake
add_test(NAME smm_core_service_tests
    COMMAND ${CMAKE_COMMAND} -E env "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            $<TARGET_FILE:smm_core_service_tests>)
```

如果你绕过 `ctest` 直接运行二进制，需要自己把 Qt 的 `bin` 加进 `PATH`。

## 回归测试写什么

优先覆盖"错了会损坏用户数据"的路径：

- id 清洗（空白 → 空、非法字符 → `-`）、改名不覆盖已有模组；
- 优先级重排的步长与顺序；
- 部署清单缺失 / 损坏时的行为（`nullopt` vs 抛 `Json`）；
- 跨卷与硬链接判定（同卷、异卷、文件数不匹配）；
- 取消：`CoreService` 在循环里看到 `CancelToken` 为真时抛 `Cancelled`；
- 体检复用已扫描结果时条目数必须与重新扫描一致（`"pre-scanned doctor reports identical items"`）。

新增断言时把夹具构造写清楚：读代码的人要能一眼看出"这个测试在模拟哪种真实故障"。

## 人工验证清单

改动 GUI 后至少走一遍：

1. 启动 → 工作台显示检测摘要，六个导航项都能切换；
2. 模组管理：搜索、分类筛选、启停、改顺位、拖拽排序、打开详情抽屉、导出单模组；
3. 导入一个 `.zip`：完成后列表出现新模组，预览图可选；
4. 部署：进度徽标出现，完成后 `deployedFiles` 增加，磁盘上是硬链接（同卷）或复制（跨卷）；
5. 还原：游戏 `mods/` 回到部署前状态，清单被删除；
6. 整合包：保存 → 应用 → 取消应用 → 导出 → 导入；
7. 环境检测页：五项都能读到结果，点「检测环境」重跑，「一键装配 / 修复 ModEngine」不报错；刻意删掉游戏目录里的 `modengine.ini` 后重跑，页面应给出异常与修复建议；
8. 设置页：改语言（立即生效）、改主题（深/浅/跟随系统）、改字体、开"减少动效"；主题切换应在一帧内完成，不应出现整窗逐帧重绘的卡顿；
9. 关闭动效后重看页面切换与卡片入场：不得有残影停在中间帧；
10. 双击 `SekiroModManager.exe`：应出现 UAC 提示，允许后标题栏与界面正常；拒绝时进程不应继续启动。

## 静态检查

```bash
qmllint -I src/gui/third_party/HuskarUI/src/imports -I src/gui/qml \
        src/gui/qml/Main.qml src/gui/qml/components/*.qml src/gui/qml/views/*.qml
```

预期零错误。`qmllint` 对 `Repeater` 委托的 `required property` 与跨文件类型引用最敏感，新组件加进 `qt_add_qml_module` 之前先跑一遍。

## CI 里怎么跑

`.github/workflows/ci.yml`：

```
configure: cmake -B build -S . -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DSMM_BUILD_TESTS=ON
build:     cmake --build build --config Release
test:      ctest --test-dir build --output-on-failure
smoke:     ./build/src/cli/smm.exe --help && ./build/src/cli/smm.exe doctor --json
```

最后一步是必要的：它验证的是"二进制能被启动、JSON 能被解析"，这是单元测试覆盖不到的一层。
