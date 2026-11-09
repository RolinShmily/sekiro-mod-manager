# 协作流程

## 分支

- 主分支 `main`，始终可构建。
- 主题分支直接用描述性前缀：`refactor/qt6-qml-migration`、`feat/armoury-grid`、`fix/hardlink-fallback`。
- CI 在 `push` 到 `main` 与 `refactor/*`、以及所有指向 `main` 的 PR 上运行；`concurrency` 组按 `workflow-ref` 取消进行中的旧构建。

## 提交信息

观察现有历史的写法：`type(scope): 一句话概括`，正文解释**为什么**。

```
build: give each component its own vendored libraries

Root third_party/ mixed two unrelated things: the libraries the core links
(nlohmann/json, miniz) and the GUI's UI toolkit submodule. Vendors now live
next to the code that uses them: ...

Moving the submodule needed the plumbing, not just a rename: .gitmodules,
.git/config, .git/modules/... and the five nested submodules ... were remapped

Verified: fresh configure + build with GUI and tests (ctest 4/4, no warnings
from our sources) and a BUILD_GUI=OFF configure + build with the two core
suites (ctest 2/2).
```

- 用到的 type：`feat`、`fix`、`refactor`、`build`、`docs`、`chore`、`release`、`ci`。
- scope 用目录或领域名：`gui`、`core`、`cli`、`website`、`ci`。
- 正文写取舍与验证方式。**"Verified:"** 段落不是形式主义：它告诉 reviewer 你实际跑过什么。
- 一个提交做一件事。重构与功能混在一起时，reviewer 无法判断哪部分是行为改变。

## 提交前

```bash
cmake --preset msvc-x64-debug -DSMM_BUILD_TESTS=ON
cmake --build --preset build-debug
ctest --test-dir build/msvc-x64-debug --output-on-failure
```

再确认：

- [ ] 没有把 `build/`、`*.exe`、`staging/`、`.smm_settings.json` 之类加进暂存区（`.gitignore` 已覆盖，但 `git add <file>` 时仍要看清）。
- [ ] 没有提交 `src/gui/third_party/HuskarUI` 内部的改动（那是配置期补丁的产物）。
- [ ] 改了 CLI 参数 / JSON 字段 / 磁盘格式 / QML 组件公开属性时，[../reference/](../reference/README.md) 或 [../design/](../design/README.md) 已同步。
- [ ] 用户可见文案已加 `qsTr()`，并更新了 `src/gui/translations/smm_en_US.ts`。
- [ ] 逻辑放在 `smm_core`，CLI 与 GUI 不会各写一份。

## PR

模板在 `.github/PULL_REQUEST_TEMPLATE.md`，勾选项与本分册 `build.md` / `testing.md` 里的命令一一对应：配置+构建、`ctest`、`smm doctor --json`、QML 改动跑 `qmllint`、手工走一遍受影响的流程。模板同时要求同一提交内同步 `docs/reference/` 下的契约文档。

PR 描述里应包含：

1. 改了什么、为什么（链接 issue）。
2. 影响面：是否改变磁盘契约、JSON 契约或用户可见行为。
3. 验证：贴 `ctest` 结果与手工验证步骤。

## CI

`.github/workflows/ci.yml`：

| 步骤 | 内容 |
|---|---|
| Checkout | `submodules: recursive` |
| Setup MSVC / Ninja / Qt | Qt 6.8.0 MSVC 2022 64-bit |
| Patch Submodules | `python scripts/patch_huskarui.py` |
| Configure | `-DSMM_BUILD_TESTS=ON`，Release |
| Build | `cmake --build build --config Release` |
| Test | `ctest --test-dir build --output-on-failure` |
| CLI Smoke Test | `smm.exe --help`、`smm.exe doctor --json` |

`.github/workflows/release.yml` 额外做：SemVer 标签校验、标签与 `project(... VERSION ...)` 一致性校验、Inno Setup 安装包、附件与 GitHub Release 发布。发布版本的唯一事实来源是 `CMakeLists.txt` 的 `project(VERSION)`。

## 文档规则

- 文档是提交的一部分，不是后续任务。
- 正文中文；代码、路径、命令原样保留。
- 交叉引用用相对路径（`../reference/cli.md`），不要写死域名或分支名。
- 单篇 60–180 行，超出就拆分。
- 写"为什么"：取舍与风险放 [../architecture/decisions.md](../architecture/decisions.md)，实现细节留在代码注释里。
