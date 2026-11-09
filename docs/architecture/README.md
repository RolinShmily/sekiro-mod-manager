# 架构分册

本分册回答"代码为什么这样分层"，不回答"这个函数做了什么"（那属于 [../reference/core-api.md](../reference/core-api.md)）。

## 依赖总图

```
                    smm_core（静态库，纯 C++17，不链接 Qt）
                     ^                        ^
       链接            |                        |            链接
                smm.exe（CLI）            smm_gui.exe（Qt 6 / QML + HuskarUI）
```

- `src/core` 只依赖标准库、`nlohmann/json` 与 `miniz`，两者都放在 `src/core/third_party/`。
- `src/cli` 只链接 `smm_core`，不链接 Qt，也不启动 GUI。
- `src/gui` 链接 `smm_core` 与 `HuskarUI`（`src/gui/third_party/HuskarUI`，git submodule）。
- 两个入口是独立进程，共享同一个核心库；GUI 不 shell 调用 CLI，CLI 不加载 GUI。

## 五条硬约束

1. **核心不打印、不弹窗、不碰 Qt。** `smm_core` 通过返回值和 `smm::SmmError` 表达结果，输出格式由调用方决定。
2. **GUI 不重复实现业务规则。** 扫描、冲突裁决、部署计划、预设应用都在核心；GUI 只做 Qt 类型映射与状态编排。
3. **CLI 不改 GUI 的私有状态。** 两边唯一的共享面是磁盘契约（[../reference/file-formats.md](../reference/file-formats.md)）与 JSON 契约（[../reference/json-contract.md](../reference/json-contract.md)）。
4. **第三方代码不享受本项目的编译选项。** `/W4` 通过 `smm::warnings` 接口目标显式挂到自有目标上；供应商目标单独 `/w`。
5. **一个用户动作对应一次核心流程。** 需要多步时，编排写在 `CoreService`，不散落在 QML 或 `main.cpp`。

## 目录

- [layers.md](layers.md) — 三层边界、依赖方向、允许与禁止、新代码该放哪
- [gui-backend.md](gui-backend.md) — GUI Backend 的五种角色与文件布局
- [async-threading.md](async-threading.md) — TaskRunner、串行化、取消、生命周期
- [data-flows.md](data-flows.md) — 导入 / 部署 / 预设 / 诊断四条主链路
- [decisions.md](decisions.md) — 已做的取舍、被否掉的方案、遗留问题

## 怎么读

- 改结构、加模块、做 code review：本页 → [layers.md](layers.md) → [decisions.md](decisions.md)。
- 改 GUI：先看 [gui-backend.md](gui-backend.md)，再确认 [async-threading.md](async-threading.md) 的线程边界有没有被越过。
- 排查"模组没生效"：直接看 [data-flows.md](data-flows.md) 的部署链路。
