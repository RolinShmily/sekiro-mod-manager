# SMM 文档

四套规范，按"要不要动手、动的是哪一层"划分。正文统一使用中文；代码、路径、命令保持原样。

| 分册 | 回答什么问题 | 谁该读 |
|---|---|---|
| [architecture/](architecture/README.md) | 代码为什么这样分层？Core / CLI / GUI 的边界在哪？异步与线程怎么约束？ | 改结构、加模块、做 code review 的人 |
| [design/](design/README.md) | 界面用什么 token、组件、动效与文案规则？分页与空状态怎么处理？ | 写 QML、调 UI、做视觉验收的人 |
| [development/](development/README.md) | 怎么配置与构建？测试跑哪些？提交与 PR 有什么约定？ | 每天写代码、跑 CI 的人 |
| [reference/](reference/README.md) | CLI 有哪些命令与退出码？JSON 契约长什么样？磁盘上写了哪些文件？ | 写脚本、接 GUI、排查现场数据的人 |

## 建议阅读顺序

- **新加入项目**：本页 → [development/build.md](development/build.md) → [architecture/layers.md](architecture/layers.md) → 需要改哪层再翻对应分册。
- **只改界面**：[design/README.md](design/README.md) → [design/components.md](design/components.md) → [design/i18n-accessibility.md](design/i18n-accessibility.md)。
- **只写后端/CLI**：[architecture/layers.md](architecture/layers.md) → [reference/cli.md](reference/cli.md) → [reference/errors.md](reference/errors.md)。
- **排查"模组没生效"**：[reference/file-formats.md](reference/file-formats.md) → [architecture/data-flows.md](architecture/data-flows.md)。

## 约定

- 文档里出现的每个路径、命令、字段都能在仓库里找到出处；改动代码时同一提交内更新对应文档。
- 交叉引用一律使用相对路径（`../architecture/layers.md`），不要写死域名或分支名。
- 单篇控制在 60–180 行：超出就拆分文件，不要堆成一个巨型页面。
- 记录"为什么"而不只是"是什么"：取舍与风险写在 [architecture/decisions.md](architecture/decisions.md)，实现细节留在代码注释里。

## 分册目录

### architecture

- [README.md](architecture/README.md) — 分册导读与依赖总图
- [layers.md](architecture/layers.md) — 三层边界、依赖方向、允许与禁止
- [gui-backend.md](architecture/gui-backend.md) — GUI Backend 的五种角色与文件布局
- [async-threading.md](architecture/async-threading.md) — TaskRunner、串行化、取消、生命周期
- [data-flows.md](architecture/data-flows.md) — 导入 / 部署 / 预设 / 诊断四条主链路
- [decisions.md](architecture/decisions.md) — 已做的取舍、被否掉的方案、遗留问题

### design

- [README.md](design/README.md) — 设计分册导读
- [screenshots.md](design/screenshots.md) — 五个页面的实拍与重新生成方式
- [tokens.md](design/tokens.md) — 颜色、字体、间距、圆角、动效时长
- [components.md](design/components.md) — PageHeader / SectionCard / ModCard / NavItem 等复用组件
- [motion.md](design/motion.md) — 动效预算、缓动、减少动效
- [i18n-accessibility.md](design/i18n-accessibility.md) — 中英双语、键盘可达、对比度、焦点

### development

- [README.md](development/README.md) — 开发分册导读
- [build.md](development/build.md) — 环境、预设、子模块、增量构建
- [code-style.md](development/code-style.md) — C++ / QML / CMake 书写约定
- [testing.md](development/testing.md) — 测试分层、运行方式、故障排查
- [workflow.md](development/workflow.md) — 分支、提交信息、PR 检查清单、CI

### reference

- [README.md](reference/README.md) — 参考分册导读
- [cli.md](reference/cli.md) — 命令、选项、示例、退出码
- [json-contract.md](reference/json-contract.md) — `--json` 信封与各命令负载
- [errors.md](reference/errors.md) — `ErrorCode` 全表与处理约定
- [file-formats.md](reference/file-formats.md) — 元数据、预设、清单、`.smmpack`、`settings.json`、`modengine.ini`
- [core-api.md](reference/core-api.md) — `smm_core` 公开头文件索引
