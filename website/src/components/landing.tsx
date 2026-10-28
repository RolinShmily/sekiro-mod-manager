"use client";

import * as Accordion from "@radix-ui/react-accordion";
import {
  ArrowRight,
  ArrowUpRight,
  Download,
  Link,
  ShieldCheck,
  FolderArchive,
  Layers,
  Plus,
  Network,
  Cpu,
  Sparkles,
  ArrowUp,
  FileQuestion,
  BookOpen,
} from "lucide-react";
import Image from "next/image";
import { useI18n } from "@/components/i18n";
import { Navbar } from "@/components/navbar";
import { Footer } from "@/components/footer";
import { Showcase } from "@/components/showcase";
import { Workflow } from "@/components/workflow";
import { Downloads } from "@/components/downloads";
import { RevealProvider } from "@/components/reveal";
import { assetPath, REPOSITORY } from "@/lib/paths";
import {
  BrandLogo,
  WindowsIcon,
  GithubIcon,
  NextjsIcon,
  ReactIcon,
  TypescriptIcon,
  TailwindIcon,
  CloudflareIcon,
  RadixIcon,
  MotionIcon,
} from "./icons";
import type { MessageKey } from "@/locales/zh";

const features = [
  {
    Icon: Link,
    number: "01",
    title: "featureLinkTitle",
    description: "featureLinkDescription",
    tag: "NTFS HARD LINKS",
  },
  {
    Icon: ShieldCheck,
    number: "02",
    title: "featureConflictTitle",
    description: "featureConflictDescription",
    tag: "CRITICAL / WARNING / INFO",
  },
  {
    Icon: FolderArchive,
    number: "03",
    title: "featureImportTitle",
    description: "featureImportDescription",
    tag: "ZIP / 7Z / RAR",
  },
  {
    Icon: Layers,
    number: "04",
    title: "featurePackTitle",
    description: "featurePackDescription",
    tag: "PRESETS / .SMMPACK",
  },
] as const;

const stack = [
  { label: "Next.js", Icon: NextjsIcon, url: "https://nextjs.org/" },
  { label: "React", Icon: ReactIcon, url: "https://react.dev/" },
  { label: "TypeScript", Icon: TypescriptIcon, url: "https://www.typescriptlang.org/" },
  { label: "Tailwind CSS", Icon: TailwindIcon, url: "https://tailwindcss.com/" },
  { label: "Radix UI", Icon: RadixIcon, url: "https://www.radix-ui.com/" },
  { label: "Lucide", Icon: Network, url: "https://lucide.dev/" },
  { label: "Motion", Icon: MotionIcon, url: "https://motion.dev/" },
  { label: "Cloudflare", Icon: CloudflareIcon, url: "https://workers.cloudflare.com/" },
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
          <p className="eyebrow hero-eyebrow">
            <span />
            FOR SEKIRO: SHADOWS DIE TWICE
          </p>
          <div className="hero-product flex items-center gap-2">
            <BrandLogo className="w-5 h-5 shrink-0" />
            <span>
              SEKIRO <span>MOD MANAGER</span>
            </span>
          </div>
          <h1 id="hero-title">
            <span>{t("heroTitle")}</span>
            <br />
            <em>{t("heroHighlight")}</em>
          </h1>
          <p className="hero-description">{t("heroDescription")}</p>
          <div className="button-row">
            <a className="button button-primary" href="#download">
              <Download className="icon" />
              <span>{t("heroDownload")}</span>
            </a>
            <a className="button button-ghost" href="#showcase">
              <span>{t("heroExplore")}</span>
              <ArrowRight className="icon" />
            </a>
          </div>
          <p className="platform">
            <span className="flex items-center gap-1.5">
              <WindowsIcon className="w-3.5 h-3.5 text-gold inline" />
              Windows 10 / 11
            </span>
            <span>·</span>
            <span className="flex items-center gap-1">
              <Cpu size={14} className="text-gold" />
              x64
            </span>
            <span>·</span>
            <span className="flex items-center gap-1">
              <ShieldCheck size={14} className="text-[#68d391]" />
              {t("openSource")}
            </span>
          </p>
        </div>
        <div className="hero-side" aria-hidden="true">
          MOD YOUR JOURNEY <span>01 — 05</span>
        </div>
        <a className="hero-window" href="#showcase" aria-label={t("heroExplore")}>
          <div className="hero-window-header">
            <div className="window-dots" aria-hidden="true">
              <span className="dot dot-close" />
              <span className="dot dot-min" />
              <span className="dot dot-max" />
            </div>
            <span className="hero-window-title">Sekiro Mod Manager</span>
          </div>
          <Image
            src={assetPath(`/assets/app/armoury-${locale}.webp`)}
            width={1360}
            height={880}
            alt={t("screenArmoury")}
          />
          <span>
            <span className="caption-dot" />
            {t("actualScreenshot")}
            <ArrowUpRight size={14} />
          </span>
        </a>
        <div className="hero-bottom container">
          <a href="#showcase" className="scroll-cue">
            <span className="scroll-line" />
            <span>{t("scroll")}</span>
          </a>
          <p>{t("heroBottom")}</p>
        </div>
      </section>
      <div className="project-band">
        <div className="container">
          <span className="flex items-center gap-2">
            <BrandLogo className="w-3.5 h-3.5 inline shrink-0" />
            SEKIRO MOD MANAGER
          </span>
          <span>{t("band")}</span>
          <a href={`${REPOSITORY}/blob/main/LICENSE`} target="_blank" rel="noopener noreferrer">
            <ShieldCheck size={14} className="text-gold mr-1" />
            MIT LICENSE <ArrowUpRight className="icon" />
          </a>
        </div>
      </div>
      <Showcase />
      <section
        className="features section-pad"
        style={{ backgroundImage: `url(${assetPath("/assets/ink.webp")})` }}
        id="features"
        aria-labelledby="features-title"
      >
        <div className="container">
          <div className="section-heading reveal">
            <p className="eyebrow">02 / BUILT FOR YOUR JOURNEY</p>
            <div className="heading-row">
              <h2 id="features-title">{t("featuresTitle")}</h2>
              <p>{t("featuresDescription")}</p>
            </div>
          </div>
          <div className="feature-grid">
            {features.map((feature, index) => (
              <article className="feature reveal" key={feature.title}>
                <div className="feature-top">
                  <feature.Icon className="icon" />
                  <span className="feature-num-badge">0{index + 1}</span>
                </div>
                <h3>{t(feature.title)}</h3>
                <p>{t(feature.description)}</p>
                <span className="feature-tag flex items-center gap-1.5">
                  <Sparkles size={11} className="text-gold shrink-0" />
                  <span>{feature.tag}</span>
                </span>
              </article>
            ))}
          </div>
        </div>
      </section>
      <section
        className="journey-banner"
        style={{ backgroundImage: `url(${assetPath("/assets/ashina.webp")})` }}
        aria-label={t("journeyArtwork")}
      >
        <div className="journey-shade" />
        <div className="container reveal">
          <p className="eyebrow">YOUR WORLD. YOUR WAY.</p>
          <p className="journey-quote">{t("journeyQuote")}</p>
          <span>{t("journeySub")}</span>
        </div>
        <span className="art-credit">SEKIRO: SHADOWS DIE TWICE</span>
      </section>
      <Workflow />
      <Downloads />
      <section className="faq section-pad" id="faq" aria-labelledby="faq-title">
        <div className="container faq-layout">
          <div className="reveal">
            <p className="eyebrow">05 / BEFORE YOU SET OUT</p>
            <h2 id="faq-title">{t("faqTitle")}</h2>
            <p>{t("faqDescription")}</p>
          </div>
          <Accordion.Root className="faq-list reveal" type="single" collapsible>
            {[1, 2, 3, 4, 5].map((index) => (
              <Accordion.Item className="faq-item" key={index} value={String(index)}>
                <Accordion.Header>
                  <Accordion.Trigger className="faq-trigger">
                    <span className="flex items-center gap-3">
                      <FileQuestion size={18} className="text-accent shrink-0" />
                      <span>{t(`faq${index}Title` as MessageKey)}</span>
                    </span>
                    <Plus className="icon faq-plus-icon" />
                  </Accordion.Trigger>
                </Accordion.Header>
                <Accordion.Content className="faq-content">
                  <p>{t(`faq${index}Description` as MessageKey)}</p>
                </Accordion.Content>
              </Accordion.Item>
            ))}
          </Accordion.Root>
        </div>
      </section>
    </main>
    <Footer />
    <noscript><p className="noscript-note">软件下载 / Software downloads: <a href="https://github.com/RolinShmily/sekiro-mod-manager/releases">GitHub Releases</a></p></noscript>
  </RevealProvider>;
}
