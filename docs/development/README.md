# 开发分册

面向每天写代码、跑构建与 CI 的人。设计约束在 [../design/](../design/README.md)，结构约束在 [../architecture/](../architecture/README.md)。

## 目录

- [build.md](build.md) — 环境、预设、子模块、增量构建、常见失败
- [code-style.md](code-style.md) — C++ / QML / CMake 书写约定
- [testing.md](testing.md) — 测试分层、运行方式、故障排查
- [workflow.md](workflow.md) — 分支、提交信息、PR 检查清单、CI

## 五分钟上手

```bash
git clone --recurse-submodules https://github.com/RolinShmily/sekiro-mod-manager.git
cd sekiro-mod-manager
cmake --preset msvc-x64-debug
cmake --build --preset build-debug
ctest --test-dir build/msvc-x64-debug --output-on-failure
```

需要 `SMM_BUILD_TESTS=ON` 才会有 `ctest` 目标，见 [build.md](build.md)。

只改核心与 CLI、机器上没有 Qt：

```bash
cmake --preset core-only
cmake --build --preset build-core
```

## 三个仓库级约定

1. **同一提交内更新文档。** 改了 CLI 参数、JSON 字段、磁盘文件或 QML 组件的公开属性，对应分册必须一起改，否则下一次有人照着文档写脚本就会失败。
2. **不要在 `src/core` 引入 Qt，也不要在 QML 里做业务判断。** 理由见 [../architecture/layers.md](../architecture/layers.md)。
3. **提交前跑 `ctest`。** 编译通过不等于规则正确；`smm_core_business_tests` 覆盖的正是"改名、优先级、清单、跨卷"这些会损坏用户数据的路径。
