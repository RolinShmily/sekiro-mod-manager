"use client";

import * as Accordion from "@radix-ui/react-accordion";
import { ArrowRight, ArrowUpRight, Download, Monitor, Link, ShieldCheck, Folder, Layers, Plus, Code, Atom, Braces, Palette, Component, Network, Activity, Globe } from "lucide-react";
import Image from "next/image";
import { useI18n } from "@/components/i18n";
import { Navbar, Brand } from "@/components/navbar";
import { Showcase } from "@/components/showcase";
import { Workflow } from "@/components/workflow";
import { Downloads } from "@/components/downloads";
import { RevealProvider } from "@/components/reveal";
import { assetPath, REPOSITORY } from "@/lib/paths";
import type { MessageKey } from "@/locales/zh";

const features = [
  { Icon: Link, number: "壹 / 01", title: "featureLinkTitle", description: "featureLinkDescription", tag: "NTFS HARD LINKS" },
  { Icon: ShieldCheck, number: "贰 / 02", title: "featureConflictTitle", description: "featureConflictDescription", tag: "CRITICAL / WARNING / INFO" },
  { Icon: Folder, number: "叁 / 03", title: "featureImportTitle", description: "featureImportDescription", tag: "ZIP / 7Z / RAR" },
  { Icon: Layers, number: "肆 / 04", title: "featurePackTitle", description: "featurePackDescription", tag: "PRESETS / .SMMPACK" },
] as const;
const stack = [
  { label: "Next.js", Icon: Code, url: "https://nextjs.org/" },
  { label: "React", Icon: Atom, url: "https://react.dev/" },
  { label: "TypeScript", Icon: Braces, url: "https://www.typescriptlang.org/" },
  { label: "Tailwind CSS", Icon: Palette, url: "https://tailwindcss.com/" },
  { label: "Radix UI", Icon: Component, url: "https://www.radix-ui.com/" },
  { label: "Lucide", Icon: Network, url: "https://lucide.dev/" },
  { label: "Motion", Icon: Activity, url: "https://motion.dev/" },
  { label: "Cloudflare Workers", Icon: Globe, url: "https://workers.cloudflare.com/" },
];

export function Landing() {
  const { t, locale } = useI18n();
  return <RevealProvider>
    <a className="skip-link" href="#main">{t("skip")}</a>
    <Navbar />
    <main id="main">
      <section className="hero" id="top" aria-labelledby="hero-title">
        <Image className="hero-art" src={assetPath("/assets/wolf.webp")} alt="" fill priority sizes="100vw" />
        <div className="hero-shade" aria-hidden="true" />
        <div className="hero-copy container">
          <p className="eyebrow hero-eyebrow"><span />FOR SEKIRO: SHADOWS DIE TWICE</p>
          <p className="hero-product">SEKIRO <span>MOD MANAGER</span></p>
          <h1 id="hero-title"><span>{t("heroTitle")}</span><br /><em>{t("heroHighlight")}</em></h1>
          <p className="hero-description">{t("heroDescription")}</p>
          <div className="button-row"><a className="button button-primary" href="#download"><Download className="icon" /><span>{t("heroDownload")}</span></a><a className="button button-ghost" href="#showcase"><span>{t("heroExplore")}</span><ArrowRight className="icon" /></a></div>
          <p className="platform"><Monitor className="icon" />Windows 10 / 11 · x64 <span>—</span><span>{t("openSource")}</span></p>
        </div>
        <div className="hero-side" aria-hidden="true">MOD YOUR JOURNEY <span>01 — 05</span></div>
        <a className="hero-window" href="#showcase" aria-label={t("heroExplore")}><Image src={assetPath(`/assets/app/armoury-${locale}.webp`)} width={1360} height={880} alt={t("screenArmoury")} /><span><span className="caption-dot" />{t("actualScreenshot")}<ArrowUpRight size={14} /></span></a>
        <div className="hero-bottom container"><a href="#showcase" className="scroll-cue"><span className="scroll-line" /><span>{t("scroll")}</span></a><p>{t("heroBottom")}</p></div>
      </section>
      <div className="project-band"><div className="container"><span>SEKIRO MOD MANAGER</span><span>{t("band")}</span><a href={`${REPOSITORY}/blob/main/LICENSE`} target="_blank" rel="noopener noreferrer">MIT LICENSE <ArrowUpRight className="icon" /></a></div></div>
      <Showcase />
      <section className="features section-pad" style={{ backgroundImage: `url(${assetPath("/assets/ink.webp")})` }} id="features" aria-labelledby="features-title"><div className="container">
        <div className="section-heading reveal"><p className="eyebrow">02 / BUILT FOR YOUR JOURNEY</p><div className="heading-row"><h2 id="features-title">{t("featuresTitle")}</h2><p>{t("featuresDescription")}</p></div></div>
        <div className="feature-grid">{features.map((feature, index) => <article className="feature reveal" key={feature.title}><div className="feature-top"><feature.Icon className="icon" /><span>{locale === "zh" ? feature.number : `0${index + 1}`}</span></div><h3>{t(feature.title)}</h3><p>{t(feature.description)}</p><span className="feature-tag">{feature.tag}</span></article>)}</div>
      </div></section>
      <section className="journey-banner" style={{ backgroundImage: `url(${assetPath("/assets/ashina.webp")})` }} aria-label={t("journeyArtwork")}><div className="journey-shade" /><div className="container reveal"><p className="eyebrow">YOUR WORLD. YOUR WAY.</p><p className="journey-quote">{t("journeyQuote")}</p><span>{t("journeySub")}</span></div><span className="art-credit">SEKIRO: SHADOWS DIE TWICE</span></section>
      <Workflow />
      <Downloads />
      <section className="faq section-pad" id="faq" aria-labelledby="faq-title"><div className="container faq-layout"><div className="reveal"><p className="eyebrow">05 / BEFORE YOU SET OUT</p><h2 id="faq-title">{t("faqTitle")}</h2><p>{t("faqDescription")}</p></div>
        <Accordion.Root className="faq-list reveal" type="single" collapsible>{[1, 2, 3, 4, 5].map(index => <Accordion.Item className="faq-item" key={index} value={String(index)}><Accordion.Header><Accordion.Trigger className="faq-trigger">{t(`faq${index}Title` as MessageKey)}<Plus className="icon" /></Accordion.Trigger></Accordion.Header><Accordion.Content className="faq-content"><p>{t(`faq${index}Description` as MessageKey)}</p></Accordion.Content></Accordion.Item>)}</Accordion.Root>
      </div></section>
    </main>
    <footer className="site-footer"><div className="container">
      <div className="footer-top"><Brand /><p>{t("footerTagline")}</p><a href={REPOSITORY} target="_blank" rel="noopener noreferrer">GitHub <ArrowUpRight className="icon" /></a></div>
      <div className="tech-stack"><span>{locale === "zh" ? "网站技术栈" : "Built with"}</span><div className="flex flex-wrap gap-2">{stack.map(item => <a key={item.label} href={item.url} target="_blank" rel="noopener noreferrer" className="inline-flex items-center gap-2 border border-white/15 px-3 py-2 text-xs hover:bg-white/5"><item.Icon size={14} />{item.label}</a>)}</div></div>
      <div className="footer-bottom"><p>{t("footerDisclaimer")}</p><div><a href={`${REPOSITORY}/blob/main/LICENSE`} target="_blank" rel="noopener noreferrer">MIT License</a><a href={assetPath("/ASSETS.md")}>{t("assetCredits")}</a><a href="#top">{t("backTop")}</a></div></div>
      <p className="footer-credit">© 2019, 2020 FromSoftware, Inc. · SMM by Rolin</p>
    </div></footer>
    <noscript><p className="noscript-note">软件下载 / Software downloads: <a href="https://github.com/RolinShmily/sekiro-mod-manager/releases">GitHub Releases</a></p></noscript>
  </RevealProvider>;
}
