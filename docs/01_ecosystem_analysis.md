# 01. 只狼 Mod 生态调研与底层技术事实

> 本文档详细解构《只狼：影逝二度》（Sekiro: Shadows Die Twice）的 Mod 底层加载机理、官方资产打包范式、常见文件级冲突机理，并复盘现有方案的不足与教训。

---

## 1. 只狼 Mod 底层加载与劫持机理

在 FromSoftware 的游戏引擎架构中，只狼所有原始游戏资产被打包并加密存储在游戏根目录的若干大型压缩包内（`Data0.bdt` ~ `Data5.bdt`，并由对应的 `.bhd` 头文件维护索引）。官方并未提供官方 Mod API 或装载接口。当前整个只狼 Mod 生态的基石完全建立在 **Sekiro Mod Engine**（当前稳定终版为 v0.1.16，作者 Katalash）之上。

```
+-------------------------------------------------------------+
|                     sekiro.exe 启动                         |
+-------------------------------------------------------------+
                              | (Windows 动态链接库搜索顺序)
                              v
             [ 加载根目录伪造的 dinput8.dll ]
                              |
       +----------------------+-----------------------+
       |                                              |
       v                                              v
[ DirectInput8Create 导出转发 ]             [ Mod Engine 核心引导 ]
(透传给系统真实 DirectInput8)                  |
                                              v
                                   [ 内存 AOB 特征码扫描 ]
                                   定位文件 I/O 路由函数
                                              |
                                              v
                                   [ Hook: 拦截 CreateFileW ]
                                              |
             +--------------------------------+--------------------------------+
             |                                                                 |
             v (存在松散文件)                                                  v (不存在)
+-----------------------------------------+                 +-----------------------------------------+
| 重定向至 modOverrideDirectory (\\mods)   |                 | 穿透回底层，读取 DataX.bdt 加密原始资产  |
+-----------------------------------------+                 +-----------------------------------------+
```

### 1.1 DLL 劫持 (DLL Search Order Hijacking)
- 只狼作为 DirectX 11 应用程序，运行时需要初始化输入设备，默认调用系统动态链接库 `dinput8.dll`。
- Sekiro Mod Engine 将自身编译为同名伪装库 `dinput8.dll` 置于 `sekiro.exe` 同级目录。利用 Windows 优先搜索应用当前目录的规则，优先截获加载流程。
- Mod Engine 内部导出 `DirectInput8Create`，在完成自身初始化后，将实际的输入处理请求透明代理至真正的 `C:\Windows\System32\dinput8.dll`，保证游戏输入逻辑不受干扰。

### 1.2 AOB 内存特征码扫描与 I/O 钩子
- Mod Engine 在游戏初始化阶段扫描主进程代码段的 AOB（Array of Bytes）特征码，定位游戏底层解构虚拟文件系统（DVDBND / Data Binder）的寻址逻辑。
- 关键拦截点在于 **`CreateFileW`**（或内部路径解析例程）。当游戏请求加载某个资产路径（例如 `parts/am_m_9000.partsbnd.dcx`）时，Mod Engine 会优先拼接检测配置的重定向目录（默认为 `sekiro.exe` 同级的 `mods/` 目录）。
- **命中机制**：若 `mods/parts/am_m_9000.partsbnd.dcx` 在磁盘上实际存在，则重定向打开该松散文件（Loose File）；若不存在，则放行原有逻辑，从 `DataX.bdt` 中解密读取官方原始资产。

### 1.3 核心技术硬约束：单目录覆盖无链式加载
- **无多目录级联支持**：与后续基于 Mod Engine 2（ME2，针对《艾尔登法环》）的多目录挂载与链式优先级（chained profiles）不同，**Sekiro Mod Engine v0.1.16 只支持单一目录挂载**。其配置文件 `modengine.ini` 的关键字段如下：
  ```ini
  [files]
  enabled=1
  loadUXMFiles=0
  cachePaths=1
  modOverrideDirectory="\mods"
  ```
- **技术推论**：任何管理器都无法通过动态修改 `modengine.ini` 并配置多个 `mod_dir_1;mod_dir_2` 来实现分层加载。**所有最终启用的 Mod 文件，在游戏运行时必须统一汇聚并存在于 `sekiro.exe` 根目录下的单一目标目录（通常为 `mods/`）内**。
- **UXM 工具已被主流弃用**：早期社区采用 UXM 解包 15GB 的整个游戏原始资产后覆盖修改，不仅极易损坏游戏本体、占用双倍磁盘空间（30GB+），且无法卸载干净，现已完全被 Mod Engine 的松散覆盖机制取代。

---

## 2. 关键资产目录规范与文件清单

只狼采用 FromSoftware 自研格式打包。所有松散资产在 `mods/` 下必须保持严格的大小写无关相对路径：

| 资产目录 | 作用与涵盖内容 | 关键代表性文件 | 特性与冲突风险 |
| :--- | :--- | :--- | :--- |
| `parts/` | 主角外观部件、武器部件、忍义手部件 | `am_m_9000.partsbnd.dcx` (只狼手臂)<br>`bd_m_9000.partsbnd.dcx` (只狼身体)<br>`hd_m_9000.partsbnd.dcx` (只狼头部)<br>`lg_m_9000.partsbnd.dcx` (只狼腿部)<br>`wp_a_0300.partsbnd.dcx` (主武器楔丸)<br>`wp_a_0310.partsbnd.dcx` (不死斩)<br>`wp_a_XXXX.partsbnd.dcx` (各类义手道具) | **最高频冲突区**。几乎所有角色换肤 Mod 均绑定在 9000 号主角槽位和 0300 号楔丸槽位，无法原生同时生效。 |
| `chr/` | 敌人、Boss、NPC、动物等模型与骨骼动画 | `c0000.chrbnd.dcx` (特定角色模型)<br>`c5110.chrbnd.dcx` (剑圣苇名一心)<br>`cXXXX.anibnd.dcx` (专属攻击动作与行为树) | 单个怪物/Boss 按编号隔离，不同角色之间不冲突；若修改同一个 Boss 外观则产生二进制互斥覆盖。 |
| `param/gameparam/` | 游戏核心数值与参数系统 | `gameparam.parambnd.dcx` | **致命冲突点**。该文件内嵌了所有的武器攻击力、躯干伤害、霸体判定、物品掉率、SpEffect 状态效果等。全游戏所有玩法向 Mod 均必须修改此文件，不可直接物理覆盖。 |
| `sound/` | 游戏音效与背景音乐 (FMOD 容器) | `fdp_main.fsb`<br>`sekiro.bank`<br>`*.fev` | FMOD 音频容器。MOD 修改通常是打包替换整个 `.fsb` 或 `.bank` 文件，无法原生进行音效级的粒度合并。 |
| `msg/` | 多语言文本与界面字符串 | `msg/engus/item.msgbnd.dcx`<br>`msg/zhocn/item.msgbnd.dcx`<br>`msg/zhotw/menu.msgbnd.dcx` | 语言分目录。国外 Mod 经常只包含 `engus/`，若未提供 `zhocn/`，中文客户端载入后新物品将显示空白或内部代码。 |
| `menu/` | UI、HUD、图标、按键提示与贴图 | `menu/hi/`、`menu/low/`<br>`01_common.tpf.dcx`<br>`menu.menubnd.dcx` | 替换 PlayStation/Xbox/Switch 按键图标或极简 HUD，文件粒度较明确，覆盖时按文件比对即可。 |
| `mtd/` | 材质着色器定义 (Material Definition) | `*.mtd` (如 `M[4].mtd`) | 高级材质着色效果，常与大型重制材质 Mod 捆绑。 |
| `event/` | 事件脚本与关卡触发逻辑 | `common.emevd.dcx`<br>`m10_00_00_00.emevd.dcx` | 控制主线任务逻辑、Boss 战触发点，大型机制 Mod 必改项。 |
| `map/` | 场景地图几何体、碰撞箱与贴图 | `map/m10/m10_00_00_00/` 等 | 场景置换与关卡重构类资产。 |

---

## 3. 常见冲突深度剖析

### 3.1 参数文件全表覆盖冲突 (`gameparam.parambnd.dcx`)
- **现象**：玩家同时安装了“武器伤害倍率平衡 Mod”与“忍具消耗纸人调整 Mod”，结果只有最后安装的一个生效，另一个彻底失效，甚至导致游戏崩溃或存档异常。
- **机理**：`gameparam.parambnd.dcx` 是一个由 FromSoftware BND4 格式封装并经过 DCX（Oodle/Kraken）压缩的大型二进制容器。其内部包含了数十张参数表（如 `EquipParamWeapon`, `AtkParam_Pc`, `CharaInitParam`, `SpEffectParam`）。**Mod Engine 是文件级别劫持，不支持二进制文件的内部流式合并**。一旦两个 Mod 均含有此文件，硬链接只能指向其中一个，后者完全被抹杀。

### 3.2 模型槽位与骨骼绑定冲突 (`am_m_9000` / `wp_a_0300`)
- **现象**：安装“2B 替换主角外观”后，再安装“雷电模型替换只狼”，外观出现身体穿模、部件错乱或覆盖。
- **机理**：只狼原生没有换装系统（除官方后期的三套回生记忆回忆幻化外）。99% 的社区人物 Mod 都选择直接替换只狼默认模型：
  - 手部：`parts/am_m_9000.partsbnd.dcx`
  - 躯干：`parts/bd_m_9000.partsbnd.dcx`
  - 头部：`parts/hd_m_9000.partsbnd.dcx`
  - 腿部：`parts/lg_m_9000.partsbnd.dcx`
  - 默认武器：`parts/wp_a_0300.partsbnd.dcx`
  这几个文件名是硬编码在只狼底层初始化装备表中的。因此，这类 Mod 具有**天然的互斥性**。

### 3.3 FMOD 音频容器的粒度割裂
- **现象**：安装“只狼打击打铁音效替换”，再安装“BGM 替换”，其中一个音效完全还原回原版。
- **机理**：音效并不是分散的 `.wav` 或 `.mp3`，而是打包进几百兆的 FMOD FSB 容器中（如 `fdp_main.fsb`）。任何音效作者输出的都是编译后的二进制库。由于缺乏动态注入工具，用户无法同时保留 Mod A 的打铁声与 Mod B 的受击声。

### 3.4 本地化语言包缺失引起的乱码或空白
- **现象**：中文玩家安装国外战斗机制重做 Mod 后，游戏内新道具或新流派招式的名称、说明均显示为乱码、`?EventText?` 或直接空白。
- **机理**：作者仅修改并打包了 `msg/engus/item.msgbnd.dcx`。当只狼设置为简体中文时，游戏引擎寻找 `msg/zhocn/item.msgbnd.dcx`，如果 `mods/` 下没有 `zhocn/` 对应的修改文件，游戏退回到官方原版资产，导致新增 ID 的文本完全不存在。

---

## 4. 现有方案复盘与教训

### 4.1 通用管理器 Vortex 的水土不服
- **多游戏过度抽象**：Vortex 试图用一套方案统一上千款游戏，导致其扩展插件（Game Extension）架构臃肿不堪。启动慢、UI 层级冗长。
- **部署模式误解**：Vortex 提供的“硬链接部署”在跨磁盘卷或权限不足时会抛出晦涩的错误，用户往往不知所措。
- **目录规范缺乏针对性**：Vortex 缺乏对只狼专属目录（`parts`, `chr`, `param`）的启发式解析，一旦 Mod 打包不标准，Vortex 往往原样部署，导致 Mod Engine 根本无法在 `mods/parts` 下找到资产，Mod 静默失效。

### 4.2 手动解压直接覆盖的“毁灭性”缺陷
- **不可逆的文件污染**：大量玩家直接将压缩包解压进游戏根目录或 `mods/`。当安装了 10 个 Mod 后，`mods/` 目录下充满了杂乱的文件，**没有任何元数据记录哪个文件来自哪个 Mod**。
- **孤儿残留**：当想要停用某个 Mod 时，只能完全删除整个 `mods/` 文件夹并重新手动解压其余 9 个 Mod；或者误删了其他 Mod 的同名共用部件，导致游戏闪退。
- **作者打包习惯极端混乱**：社区作者打包格式五花八门：
  - *范式 A*：`Archive.zip -> mods -> parts -> ...`
  - *范式 B*：`Archive.zip -> parts -> ...`
  - *范式 C*：`Archive.zip -> ModName_v1.0 -> mods -> parts -> ...`
  - *范式 D*：`Archive.zip -> am_m_9000.partsbnd.dcx`（根目录下散装文件）  
  普通玩家根本无法辨别到底该把哪一层文件夹拖入游戏目录。

---

## 5. 结论与架构推论

只狼专属 Mod 管理器必须遵循以下工程铁律：
1. **统一归一化（Archive Normalization）**：入库阶段必须自动纠偏并抹平作者各异的解压路径，强行标准化为以 `parts/`、`chr/`、`param/` 等为基准的结构。
2. **虚拟映射部署（Virtual Deployment）**：严禁采用直接物理复制，必须通过 Staging 仓库维护独立资产，通过 NTFS 硬链接向游戏 `mods/` 单一目录进行秒级投影，保持游戏目录绝对可追溯、可还原。
3. **精准预警**：建立资产黑名单与特征库，针对 `gameparam.parambnd.dcx` 和主角外观槽位实施第一时间的冲突拦截与优先级明示。
