export const zh = {
  meta: {
    title: "Sekiro Mod Manager — 只狼原生零拷贝桌面模组管理器",
  },
  nav: {
    skip: "跳到正文",
    showcase: "界面预览",
    features: "核心特性",
    download: "下载中心",
    packs: "整合包尝鲜",
    faq: "常见问题",
    language: "切换语言",
    menu: "打开导航菜单",
    close: "关闭导航菜单",
  },
  hero: {
    eyebrow: "Windows 10 / 11 x64 · 原生 C++17 / Qt 6 · MIT 开源",
    title: "《只狼》原生零拷贝桌面模组管理器",
    subtitle:
      "将模组隔离在独立暂存区，以 NTFS 硬链接瞬间投射到游戏 mods/ 目录——不占额外磁盘空间，自动识别目录与冲突顺位，随时一键纯净回滚。",
    primaryCta: "下载桌面端",
    secondaryCta: "浏览尝鲜整合包",
    githubCta: "GitHub 仓库",
  },
  showcase: {
    caption: "Sekiro Mod Manager · 桌面工作台实机截图",
    downloadLink: "前往下载",
    slides: [
      {
        id: "armoury",
        label: "模组管理",
        image: "/assets/app/armoury-zh.webp",
        summary: "可视化顺位裁决（数字越小越优先）、逐模组与逐文件开关、分类筛选与冲突详情抽屉。",
      },
      {
        id: "hub",
        label: "启动与工作台",
        image: "/assets/app/hub-zh.webp",
        summary: "汇总部署文件数、硬链接节省空间与环境健康结论；启动前自动执行五项体检。",
      },
      {
        id: "packs",
        label: "整合包预设",
        image: "/assets/app/packs-zh.webp",
        summary: "把当前启用集合与优先级保存为预设，支持导入/导出便携 .smmpack 整合包一键切换。",
      },
      {
        id: "settings",
        label: "全局配置",
        image: "/assets/app/settings-zh.webp",
        summary: "自动探测只狼目录并提示同卷 NTFS 硬链接可用性，支持深浅主题、中英双语与字体设置。",
      },
    ],
  },
  features: {
    eyebrow: "CORE CAPABILITIES",
    title: "为稳定、可逆的只狼模组管理而设计",
    subtitle: "从压缩包导入到启动游戏，每个环节都围绕「零拷贝、可追溯、不污染原版」构建。",
    items: [
      {
        id: "hardlink",
        badge: "NTFS HARD LINKS",
        title: "零拷贝部署与纯净回滚",
        description:
          "同卷通过 CreateHardLinkW 瞬间投射，不复制冗余大文件；跨卷自动回退复制并提示。每次部署记录在 .smm_manifest.json，一键还原绝不误删原版文件。",
      },
      {
        id: "normalize",
        badge: "ZIP / 7Z / RAR",
        title: "自动目录归一化",
        description:
          "兼容 Nexus 与社区包的任意层级目录，支持最多 6 层递归解包，自动将散落资产归入 parts/、chr/、param/、menu/font/ 并清理临时解包残留。",
      },
      {
        id: "conflict",
        badge: "3-TIER ARBITRATION",
        title: "三级语义冲突仲裁",
        description:
          "自动区分严重（参数平衡表）、警告（玩家与武器独占槽位）与提示（一般覆盖），按裁决顺位明确胜出文件，并支持在详情抽屉按单个资产精细开关。",
      },
      {
        id: "doctor",
        badge: "ENVIRONMENT DOCTOR",
        title: "五项环境体检与一键修复",
        description:
          "独立诊断页集中核验 sekiro.exe、ModEngine dinput8.dll、modengine.ini、暂存卷与部署一致性，按需修补配置。不内置、不修改游戏或 ModEngine 二进制。",
      },
    ],
  },
  workflow: {
    eyebrow: "WORKFLOW",
    title: "三步上手，无需手动折腾目录",
    steps: [
      {
        step: "01",
        title: "拖入模组或 .smmpack",
        description: "直接导入压缩包、文件夹或整套 .smmpack 整合包，自动完成解包归一化与元数据入库。",
      },
      {
        step: "02",
        title: "调整顺位与排错",
        description: "开启需要的模组，通过优先级决定共享文件归属；冲突徽标一目了然，单文件可单独关闭。",
      },
      {
        step: "03",
        title: "部署启动或一键还原",
        description: "通过环境体检后瞬间硬链接至 mods/ 并启动只狼；想回原版时点一次「还原纯净」即可。",
      },
    ],
  },
  download: {
    eyebrow: "DOWNLOAD CENTER",
    title: "下载桌面客户端",
    subtitle: "支持站内 Cloudflare CDN 镜像加速与 GitHub Releases 官方直连，两种渠道提供完全一致的 SHA-256 校验产物。",
    appTitle: "Sekiro Mod Manager · Windows 10 / 11 x64",
    appDesc: "原生桌面应用（动态链接 Qt 6 运行时），首次启动需确认一次 UAC 管理员权限以确保 ModEngine 正常加载。",
    sourceLabel: "下载线路",
    sourceMirror: "站内镜像加速",
    sourceGithub: "GitHub 直连",
    sourceNote: "镜像加速由本站 Cloudflare Worker 白名单流式反代至 GitHub Releases，与直连文件逐字节一致。",
    loading: "正在获取最新版本信息…",
    latest: "最新版本",
    unavailable: "暂时无法获取发布清单，可直接前往 GitHub Releases 下载",
    retry: "重试",
    allReleases: "GitHub Releases 全部版本",
    setupTitle: "Setup 安装程序 (.exe)",
    setupBadge: "推荐",
    setupDesc: "标准 Windows 安装向导，自动创建开始菜单与桌面快捷方式，支持平滑覆盖升级与完整卸载。",
    portableTitle: "便携绿色版 (.zip)",
    portableBadge: "免安装",
    portableDesc: "解压即用，内置全部精简 Qt 与 VC++ 运行时 DLL，不写系统卸载项，适合随 U 盘或独立目录存放。",
    downloadBtn: "下载",
    hashCopy: "复制 SHA-256",
    hashCopied: "已复制",
    hashFailed: "复制失败",
    modEngineNote:
      "提示：SMM 不内置也不分发 ModEngine（dinput8.dll），请自行从 Nexus Mods #6 获取原版 ModEngine 后放入游戏目录或暂存区。",
    modEngineLink: "前往 Nexus Mods 获取 ModEngine",
  },
  packs: {
    eyebrow: "COMMUNITY SHOWCASE · .SMMPACK",
    title: "社区尝鲜整合包",
    subtitle:
      "为新用户准备的三套可选社区模组组合包，下载后拖入 SMM「整合包预设」页即可一键导入、体验冲突裁决与纯净切换。",
    routeLabel: "R2 线路",
    routeCdn: "优选 CDN 加速",
    routeOrigin: "R2 源站直连",
    includedMods: "包含模组与原址",
    readyHint: "导入步骤：下载 .smmpack → 打开 SMM「整合包预设」→ 点击「导入整合包」即可自动解压并配置顺位。",
    downloadPack: "下载 .smmpack",
    pendingBtn: "R2 镜像上传中",
    pendingNote: "即将开放 R2 高速直连",
    fileLabel: "文件名",
    disclaimerTitle: "第三方社区模组版权与免责声明",
    disclaimerBody:
      "以下 .smmpack 尝鲜包由第三方社区模组组合整理，仅用于非商业性质的软件功能演示与新玩家快速上手，独立托管于外部对象存储（不包含在 SMM 源代码仓库及桌面端安装包内）。其中所有模组资产的知识产权与许可条款均完全归属各原作者（每张卡片下方均保留 Nexus Mods 原始链接），SMM 的 MIT 许可证不适用于任何社区模组内容。",
    takedownLink: "原作者署名补充 / 立即下架通道 (GitHub Issues)",
  },
  faq: {
    eyebrow: "FAQ",
    title: "常见问题",
    subtitle: "开始使用前最常遇到的几个问题。",
    items: [
      {
        q: "为什么建议将暂存区与只狼游戏目录放在同一个硬盘分区？",
        a: "NTFS 硬链接只能在同一个卷（如都在 D 盘）内创建，部署几十 GB 模组几乎瞬间完成且占用 0 额外空间。如果跨分区，SMM 会自动安全回退为文件复制并在界面提示。",
      },
      {
        q: "为什么 SMM 不直接内置 ModEngine（dinput8.dll）？",
        a: "ModEngine 保留所有权利且未授权第三方工具直接打包再分发。SMM 尊重原作者条款：仅负责检测、校验并自动修补 modengine.ini，需由玩家自行从 Nexus Mods #6 下载原版 dinput8.dll。",
      },
      {
        q: "为什么启动软件时会请求 UAC 管理员权限？",
        a: "在非提权启动链下，Steam 的 PreferSystem32 策略会导致只狼加载系统自带的 DINPUT8.dll 而静默跳过 ModEngine。统一提权启动可根除该问题，同时也方便操作 Program Files 下的游戏目录。",
      },
      {
        q: "社区尝鲜 .smmpack 整合包怎么用？会不会覆盖我已有的模组？",
        a: "打开 SMM 的「整合包预设」页导入 .smmpack 后，内部模组会按预设顺位入库，你可以随时切回旧预设或单独关闭其中任意模组。所有社区模组版权归原作者所有。",
      },
      {
        q: "想恢复原版只狼怎么办？",
        a: "点击「启动与工作台」中的「还原纯净」，SMM 会严格依据 .smm_manifest.json 仅移除自己投射进 mods/ 的链接与空目录，你的游戏本体与存档丝毫不受影响。",
      },
    ],
  },
  footer: {
    description: "专为《只狼：影逝二度》打造的原生 C++17 / Qt 6 零拷贝桌面模组管理器。",
    siteHeading: "页面导航",
    projectHeading: "项目与文档",
    project: "GitHub 仓库",
    releases: "历史版本",
    docs: "技术文档",
    blog: "作者博客",
    license: "MIT 许可证",
    notices: "第三方声明",
    webLicenses: "网站依赖许可",
    disclaimer:
      "独立开源社区工具，与 FromSoftware, Inc. 及 Activision 无关；所有第三方模组归原作者所有。",
  },
} as const;

export type Messages = typeof zh;
