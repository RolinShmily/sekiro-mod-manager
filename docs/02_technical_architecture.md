# 02. 系统技术架构与核心算法设计

> 本文档规范 Sekiro Mod Manager (SMM) 的分层架构体系、核心调度流程以及四大关键算法：NTFS 硬链接虚拟部署生命周期、压缩包智能归一化算法、冲突检测与遮蔽计算模型、跨卷容错与降级策略。

---

## 1. 整体分层架构体系

SMM 严格遵循分层解耦原则，自顶向下分为表现层、业务核心层、部署引擎层与物理存储层：

```
+-----------------------------------------------------------------------+
|                       表现层 (Presentation Layer)                      |
|          GUI (Web前端 / 原生桌面)   或   CLI (命令行交互工具)           |
|      - Mod 启停列表       - 拖拽导入区        - 冲突透视看板          |
|      - 一键部署 / 还原    - 环境自检卡片      - 优先级拖拽排序        |
+-----------------------------------+-----------------------------------+
                                    | API / IPC 契约调用
+-----------------------------------v-----------------------------------+
|                        业务核心层 (Mod Core)                          |
|  +------------------------+  +------------------------+  +---------+  |
|  |   Archive Processor    |  |    Conflict Engine     |  | Profile |  |
|  | (智能解压与指纹归一化)  |  | (三维冲突矩阵与遮蔽计算) |  | Manager |  |
|  +------------------------+  +------------------------+  +---------+  |
|  +-----------------------------------------------------------------+  |
|  |                       State & Config Store                      |  |
|  |      - mods.json (元数据/启停/哈希)   - settings.json (路径配置)    |  |
|  +-----------------------------------------------------------------+  |
+-----------------------------------+-----------------------------------+
                                    | 部署计划 (Deployment Plan)
+-----------------------------------v-----------------------------------+
|                     部署引擎层 (Deployment Engine)                    |
|  +-----------------------------------------------------------------+  |
|  |                     Link & File Orchestrator                    |  |
|  |   - 卷类型检测 (Volume Detection)   - 事务性清空/还原引擎          |  |
|  |   - Win32 CreateHardLink 调度器     - 降级分支 (Junction / Copy)  |  |
|  +-----------------------------------------------------------------+  |
+-----------------------------------+-----------------------------------+
                                    | 物理文件 I/O
+-----------------------------------v-----------------------------------+
|                        物理存储层 (Storage Layer)                      |
|                                                                       |
|   [ 隔离暂存库 Staging Repo ]                 [ 游戏运行时目标端 ]     |
|   C:/SMM_Staging/                              D:/Steam/sekiro/        |
|   ├── mod_A/root/parts/...                     ├── sekiro.exe          |
|   └── mod_B/root/param/...                     ├── dinput8.dll         |
|                                                ├── modengine.ini       |
|                                                └── mods/ (硬链接投影端)|
|                                                    ├── parts/...       |
|                                                    └── param/...       |
+-----------------------------------------------------------------------+
```

### 1.1 表现层 (Presentation Layer)
- 负责用户视觉呈现与操作捕获，不包含具体文件系统操作逻辑。
- 接收来自核心层的结构化状态数据（Mod 状态、冲突清单、部署进度），支持列表拖拽调整优先级。

### 1.2 业务核心层 (Mod Core)
- **Archive Processor**：处理原始压缩包，执行基于文件指纹的智能归一化，将杂乱的作者压缩包转化为规范化的本地存储结构。
- **Conflict Engine**：比对已启用 Mod 的所有相对文件路径，建立冲突拓扑图，计算文件遮蔽（Shadowing）与胜出方。
- **State Store**：持久化 Mod 状态、配置方案与快照。

### 1.3 部署引擎层 (Deployment Engine)
- 纯粹的底层文件系统操作执行者。
- 负责驱动 Windows 底层 API（如 `CreateHardLinkW`、`CreateSymbolicLinkW`、`CreateDirectoryW`），执行原子级清空、部署与异常回滚。

### 1.4 物理存储层 (Storage Layer)
- **Staging Repo（暂存库）**：独立于游戏外的存储目录，按 Mod ID 隔离存放归一化后的解压纯净资产。
- **Target Mods Dir（目标投影端）**：即 `Sekiro/mods/`，仅作为硬链接的挂载目标，随时可全部清空重建。

---

## 2. NTFS 硬链接虚拟部署生命周期算法

### 2.1 技术选型理由：为何选择 NTFS 硬链接？
1. **零磁盘开销**：只狼大型材质包与高模 Mod 动辄 1~3GB。硬链接（Hard Link）在 NTFS 文件分配表（MFT）中仅创建新的目录项指向相同的 File Record 索引节点，不占用任何额外磁盘空间。
2. **秒级原子部署**：创建 10,000 个硬链接通常耗时低于 0.3 秒，比起传统耗费数分钟的物理复制，体验提升数个数量级。
3. **零残留与完全解耦**：删除 `mods/` 内的硬链接文件，只会将该文件的链接引用计数减 1，绝不会影响暂存库中的原始 Mod 文件。实现 100% 干净卸载。
4. **游戏无感知原生兼容**：Mod Engine 底层调用 `CreateFileW` 打开文件时，硬链接与普通实体文件在内核层完全等价，没有任何符号链接（Symlink）可能存在的权限提权或解析穿透兼容性问题。

### 2.2 部署计划生成与生命周期流水线

```
[ 触发一键部署 ]
       |
       v
1. 校验前置环境 (Sekiro.exe / dinput8.dll / modengine.ini 是否就绪)
       |
       v
2. 提取所有 Enabled=true 的 Mod，依据用户 UI 排序生成严格优先级队列:
   [Mod_Priority_1 (最高), Mod_Priority_2, ..., Mod_Priority_N (最低)]
       |
       v
3. 逆向遍历合并，构建文件映射路由表 (TargetRelativePath -> SourceAbsolutePath):
   - 从最低优先级向最高优先级遍历注入 Map；
   - 若发生 Key 碰撞，高优先级自动覆盖（Overwrite）低优先级映射记录；
   - 记录碰撞文件生成 Conflict Report。
       |
       v
4. 清理目标端: 安全扫描清空现有 Sekiro/mods/ 目录下的所有硬链接与空子目录
       |
       v
5. 递归创建目标目录骨架 (CreateDirectoryW)
       |
       v
6. 批量调用 Win32 CreateHardLinkW 建立链接
       |
       +---> [成功] -> 标记部署状态 deployed=true，更新部署指纹锁
       |
       +---> [异常] -> 触发 Rollback 事务，回退清理，输出错误日志
```

### 2.3 数据结构：部署路由映射表

```json
{
  "deployment_timestamp": 1773280000,
  "active_profile": "default",
  "mappings": [
    {
      "target_relative_path": "parts\\wp_a_0300.partsbnd.dcx",
      "source_absolute_path": "D:\\SMM_Staging\\mod_moonlight_katana\\root\\parts\\wp_a_0300.partsbnd.dcx",
      "owner_mod_id": "mod_moonlight_katana",
      "priority": 1,
      "shadowed_mods": ["mod_bloodborne_weapons"]
    },
    {
      "target_relative_path": "param\\gameparam\\gameparam.parambnd.dcx",
      "source_absolute_path": "D:\\SMM_Staging\\mod_resurrection\\root\\param\\gameparam\\gameparam.parambnd.dcx",
      "owner_mod_id": "mod_resurrection",
      "priority": 2,
      "shadowed_mods": []
    }
  ]
}
```

---

## 3. 压缩包智能归一化算法 (Archive Normalization)

由于社区作者打包目录深浅不一，SMM 必须在解压入库阶段执行智能感知，杜绝人工搬运操作。

### 3.1 资产特征指纹库定义
算法内置只狼标准核心目录指纹与特征文件签名：

```rust
// 标准顶层锚点目录集合 (大小写无关匹配)
const SEKIRO_CANONICAL_DIRS: &[&str] = &[
    "parts", "chr", "param", "sound", "msg", 
    "mtd", "menu", "event", "map", "action", "cutscene"
];

// 特征扩展名模式与对应映射
const SEKIRO_FILE_SIGNATURES: &[(&str, &str)] = &[
    (".partsbnd.dcx", "parts"),
    (".chrbnd.dcx",   "chr"),
    (".anibnd.dcx",   "chr"),
    (".parambnd.dcx", "param/gameparam"),
    (".fsb",          "sound"),
    (".bank",         "sound"),
    (".fev",          "sound"),
    (".emevd.dcx",    "event"),
    (".tpf.dcx",      "menu"),
    (".menubnd.dcx",  "menu"),
    (".mtd",          "mtd"),
];
```

### 3.2 归一化判定树与提纯逻辑

```
输入: 解压至临时工作区的原始解压树 TempDir
                          |
                          v
         [ 扫描 TempDir 下所有目录与文件项 ]
                          |
                          +----------------------------------------------+
                          |                                              |
                          v                                              v
           [ 匹配到 SEKIRO_CANONICAL_DIRS? ]              [ 未匹配到任何标准目录项? ]
                          |                                              |
             +------------+------------+                                 v
             |                         |                      [ 扫描根目录所有文件后缀 ]
             v (存在单一/多个标准目录)  v (父级有包装名)                  |
      [该目录是否在根部?]        [定位最深公共祖先目录 Pivot]            +-------------------+
             |                         |                                 |                   |
      +------+------+                  |                                 v (命中特征扩展名)   v (未识别)
      |             |                  |                       [依据映射表自动重组]    [警告并保留原样]
      v             v                  v                       例: am_m_9000 -> parts/  提示用户手动指定
  [直接作为root] [剪切Pivot内容至root] [剪切Pivot至root]
```

#### 典型场景处理案例：
- **场景 1（嵌套作者名与 mods 目录）**：  
  压缩包内部结构：`CoolMod_v2.0/Sekiro/mods/parts/am_m_9000.partsbnd.dcx`  
  *识别判定*：扫描发现 `parts` 位于三层之后，计算其所在目录即为标准资产根；自动剥除 `CoolMod_v2.0/Sekiro/mods/`，提取 `parts/` 直接置入 Staging 的 `root/parts/`。
- **场景 2（散装文件堆积）**：  
  压缩包内部结构：`am_m_9000.partsbnd.dcx`、`wp_a_0300.partsbnd.dcx`、`readme.txt`  
  *识别判定*：根目录下无标准文件夹，但识别出 `.partsbnd.dcx`；自动新建 `parts/` 文件夹并将 `.partsbnd.dcx` 移入，`readme.txt` 移至元数据展示区。

---

## 4. 冲突检测模型与数据结构设计

### 4.1 冲突判定矩阵

```
                       +---------------------------------------+
                       |           冲突判定 (Collision)        |
                       +---------------------------------------+
                                           |
                   +-----------------------+-----------------------+
                   |                                               |
                   v                                               v
        [ 同名文件物理覆盖 ]                            [ 逻辑语义级冲突 ]
  (Exact Path Collision)                     (Semantic / Slot Collision)
                   |                                               |
         +---------+---------+                             +-------+-------+
         |                   |                             |               |
         v                   v                             v               v
   [ 普通资源冲突 ]    [ 核心参数冲突 ]             [ 主角槽位独占 ]  [ 语言包孤岛 ]
   (如 menu, mtd)   (gameparam.parambnd.dcx)       (am_m_9000 等)    (仅有 engus)
   - 策略: 优先级覆盖  - 策略: 严重高危报警          - 策略: 互斥提示   - 策略: 提示可能缺字
                       - 必须显式展示遮蔽详情        - 推荐禁用其一
```

### 4.2 冲突检测核心数据结构 (Rust/TypeScript 示意)

```rust
pub enum ConflictSeverity {
    Info,       // 普通贴图、着色器覆盖，优先级正常接管
    Warning,    // 武器/角色槽位互斥，但逻辑上可单一生效
    Critical,   // gameparam.parambnd.dcx 覆盖，导致关键游戏机制/数值被完全截断
}

pub struct ConflictRecord {
    pub relative_path: String,
    pub severity: ConflictSeverity,
    pub winner_mod_id: String,
    pub shadowed_mod_ids: Vec<String>,
    pub hint_message: String,
}

pub struct ConflictReport {
    pub has_critical_conflict: bool,
    pub records: Vec<ConflictRecord>,
}
```

---

## 5. 容错与跨卷降级策略 (Cross-Volume Strategy)

NTFS 硬链接受到 Windows 内核文件系统的物理限制：**Hard Link 只能在同一个逻辑卷/驱动器分区内创建（例如同在 `D:` 盘）**。如果用户的游戏安装在 `D:\Steam\steamapps\common\Sekiro`，而 SMM 的暂存库被配置在 `C:\Users\...\AppData`，直接调用 `CreateHardLinkW` 将返回系统错误码 `ERROR_NOT_SAME_DEVICE` (0x11)。

### 5.1 部署策略决策树

```
[ 启动部署 ]
     |
     v
[ 获取并比对 Staging 与 Sekiro/mods 所在驱动器盘符 ]
     |
     +-----------------------------------------+
     |                                         |
     v (同盘卷, 如均为 D:)                     v (跨盘卷, 如 C: 与 D:)
[ 首选: NTFS 硬链接部署 ]                      [ 触发跨卷探测与警告 ]
  - 速度: <0.5秒                                 |
  - 空间占用: 0 字节                             +-----------------------------------+
  - 稳定性: 极高 (无需特权)                      |                                   |
                                                 v (用户同意一键迁移 Staging)          v (用户坚持跨盘部署)
                                         [ 引导将 Staging 迁移至同一盘卷 ]       [ 执行降级分支策略 ]
                                         (推荐标准解法，一劳永逸恢复硬链接)         |
                                                                                   +-------+-------+
                                                                                   |               |
                                                                                   v               v
                                                                          [ 降级策略 A ]   [ 降级策略 B ]
                                                                          目录联接/软链接   全量物理复制
                                                                          (Junction)       (File Copy)
```

### 5.2 降级技术路线对比与选型

| 降级方案 | 实现机制 | 优势 | 缺陷与风险 |
| :--- | :--- | :--- | :--- |
| **首选：同盘卷引导 (Recommended)** | 侦测到跨盘时，推荐在 `Sekiro.exe` 所在分区建立 `.smm_staging/` | 100% 保持硬链接优势，零额外占用，秒级装卸 | 初次配置需一次磁盘定位引导。 |
| **降级 A：NTFS 符号链接 / 联接点 (Symlink / Junction)** | 使用 `CreateSymbolicLinkW` 或 `mklink /J` | 跨卷支持，无需物理拷贝 | Windows 默认需要管理员权限或启用“开发者模式”（Developer Mode），普通用户权限报错率高；Mod Engine 对某些软链存在路径解析穿透隐患。 |
| **降级 B：物理全量复制 (Physical Copy)** | 经典 `CopyFileW` 物理拷贝进 `mods/` | 零权限门槛，跨磁盘任意拷贝，兼容性最高 | 占用双倍磁盘空间（数 GB）；部署耗时从 0.3 秒延长至数十秒；卸载时需全量删除物理文件。 |

**设计准则**：系统默认以“**同盘卷引导**”为核心体验标准；在用户明确知晓风险且拒绝同盘迁移时，自动安全回退至“全量物理复制”并打印性能消耗提示，确保程序绝不中断崩溃。
