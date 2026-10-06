"use client";

import { useState } from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { ArrowLeft, ArrowRight, Monitor, LayoutGrid, Layers, SlidersHorizontal } from "lucide-react";
import Image from "next/image";
import { useI18n } from "./i18n";
import { assetPath } from "@/lib/paths";

const screens = [
  { id: "hub", label: "screenHub", Icon: Monitor, zh: { title: "从这里，整装出发。", description: "集中查看部署状态、环境诊断与游戏入口。准备就绪，再出征苇名。", points: ["部署、启动与环境诊断", "硬链接与磁盘占用信息", "管理页一键直达"] }, en: { title: "Prepare your next journey.", description: "Deployment state, diagnostics, and the game launcher in one place. Check the environment before you set out.", points: ["Deploy, launch, and diagnose", "Hard-link and storage information", "Quick access to your library"] } },
  { id: "armoury", label: "screenArmoury", Icon: LayoutGrid, zh: { title: "每个模组，各归其位。", description: "查看外观、武器与玩法模组。调整顺位、切换资产，把生效方案交代清楚。", points: ["卡片与列表两种视图", "模组分类、搜索与批量管理", "裁决顺位与单文件开关"] }, en: { title: "A place for every mod.", description: "Browse appearances, weapons, and gameplay. Choose priorities and individual assets to make the active plan clear.", points: ["Card and list views", "Categories, search, and batch actions", "Priorities and per-file toggles"] } },
  { id: "packs", label: "screenPacks", Icon: Layers, zh: { title: "收藏一套，随时再来。", description: "保存启用组合与顺位，在不同装配之间切换。导出整合包，分享你的苇名。", points: ["保存与应用启用组合", "导入、导出 .smmpack", "独立维护预设与模组文件"] }, en: { title: "Keep a loadout. Return anytime.", description: "Save enabled mods and their priorities, then switch between setups. Export a modpack to share your version of Ashina.", points: ["Save and apply enabled combinations", "Import and export .smmpack", "Presets separate from mod files"] } },
  { id: "settings", label: "screenSettings", Icon: SlidersHorizontal, zh: { title: "习惯，也可以自定义。", description: "选择游戏与暂存目录，切换语言和主题，搜索系统字体。让工作台适合你的使用方式。", points: ["中英双语与深浅主题", "已安装字体搜索与预览", "可保存的减少动效选项"] }, en: { title: "Make the workspace yours.", description: "Choose game and staging paths, language, theme, and installed fonts. Keep a workspace that fits the way you work.", points: ["Chinese / English and light / dark themes", "Installed-font search and preview", "Saved reduced-motion preference"] } },
] as const;

export function Showcase() {
  const { locale, t } = useI18n();
  const [active, setActive] = useState("hub");
  const index = screens.findIndex(screen => screen.id === active);
  const current = screens[index];
  const copy = current[locale];
  function change(offset: number) { setActive(screens[(index + offset + screens.length) % screens.length].id); }
  return <section className="showcase section-pad" id="showcase" aria-labelledby="showcase-title">
    <div className="container">
      <div className="section-heading reveal"><p className="eyebrow">01 / YOUR MODDING WORKSPACE</p><div className="heading-row"><h2 id="showcase-title">{t("showcaseTitle")}</h2><p>{t("showcaseDescription")}</p></div></div>
      <Tabs.Root value={active} onValueChange={setActive} className="showcase-layout reveal" orientation="horizontal">
        <div className="showcase-menu">
          <Tabs.List className="showcase-tabs" aria-label={t("screensLabel")}>{screens.map((screen, i) => <Tabs.Trigger className="showcase-tab" key={screen.id} value={screen.id} id={`tab-${screen.id}`}><span className="tab-number">0{i + 1}</span><screen.Icon className="icon" /><span>{t(screen.label)}</span><ArrowRight className="icon tab-arrow" /></Tabs.Trigger>)}</Tabs.List>
          <div className="showcase-details" aria-live="polite"><span className="mini-label">0{index + 1} / 04</span><h3>{copy.title}</h3><p>{copy.description}</p><ul>{copy.points.map(point => <li key={point}>{point}</li>)}</ul></div>
          <div className="showcase-controls"><button className="icon-button" id="screen-prev" onClick={() => change(-1)} aria-label={t("previous")}><ArrowLeft className="icon" /></button><span className="mini-label">{t("manualSwitch")}</span><button className="icon-button" id="screen-next" onClick={() => change(1)} aria-label={t("next")}><ArrowRight className="icon" /></button></div>
        </div>
        {screens.map(screen => <Tabs.Content className="showcase-visual" key={screen.id} value={screen.id}><div className="window-frame"><Image src={assetPath(`/assets/app/${screen.id}-${locale}.webp`)} alt={`${t(screen.label)} — ${t("actualScreenshot")}`} width={1360} height={880} loading="lazy" /></div><div className="screen-caption"><span><span className="caption-dot" />{t("actualScreenshot")}</span><span>C++ / Qt / QML</span></div></Tabs.Content>)}
      </Tabs.Root>
    </div>
  </section>;
}
