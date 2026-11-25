"use client";

import * as Accordion from "@radix-ui/react-accordion";
import {
  ArrowDownToLine,
  ArrowRight,
  ChevronDown,
  FolderArchive,
  Link2,
  Package,
  ShieldCheck,
  Stethoscope,
} from "lucide-react";
import { useState } from "react";
import { useI18n } from "@/components/i18n";
import { Navbar } from "@/components/navbar";
import { Downloads } from "@/components/downloads";
import { StarterPacks } from "@/components/packs";
import { Footer } from "@/components/footer";
import { assetPath, REPOSITORY } from "@/lib/paths";

const featureIcons = {
  hardlink: Link2,
  normalize: FolderArchive,
  conflict: ShieldCheck,
  doctor: Stethoscope,
} as const;

export function Landing() {
  const { t } = useI18n();
  const [slideIndex, setSlideIndex] = useState(0);

  const slides = t.showcase.slides;
  const activeSlide = slides[slideIndex] ?? slides[0];

  return (
    <div id="top" className="min-h-screen bg-canvas text-ink">
      <Navbar />

      <main id="main" className="mx-auto max-w-6xl px-4 pb-24 pt-12 sm:px-6 sm:pt-16">
        {/* 1. Focused Hero */}
        <section className="mx-auto max-w-3xl text-center">
          <div className="inline-flex items-center gap-2 rounded-full border border-accent/25 bg-accent/10 px-3.5 py-1 text-xs font-medium text-accent-hover">
            <span className="h-1.5 w-1.5 rounded-full bg-accent" aria-hidden="true" />
            {t.hero.eyebrow}
          </div>

          <h1 className="mt-6 text-balance text-3xl font-bold tracking-tight text-ink sm:text-5xl sm:leading-[1.12]">
            {t.hero.title}
          </h1>

          <p className="mx-auto mt-5 max-w-2xl text-pretty text-base leading-relaxed text-dim sm:text-lg">
            {t.hero.subtitle}
          </p>

          <div className="mt-8 flex flex-wrap items-center justify-center gap-3">
            <a
              href="#download"
              className="inline-flex h-11 items-center gap-2 rounded-xl bg-accent px-5 text-sm font-semibold text-black shadow-lg shadow-amber-500/15 transition hover:bg-accent-hover"
            >
              <ArrowDownToLine size={16} aria-hidden="true" />
              {t.hero.primaryCta}
            </a>

            <a
              href="#packs"
              className="inline-flex h-11 items-center gap-2 rounded-xl border border-line-strong bg-surface px-5 text-sm font-semibold text-ink transition hover:border-white/25 hover:bg-card"
            >
              <Package size={16} className="text-accent" aria-hidden="true" />
              {t.hero.secondaryCta}
              <ArrowRight size={15} className="text-muted" aria-hidden="true" />
            </a>

            <a
              href={REPOSITORY}
              target="_blank"
              rel="noreferrer"
              className="inline-flex h-11 items-center gap-2 rounded-xl border border-line bg-canvas px-4 text-sm font-medium text-dim transition hover:border-line-strong hover:text-ink"
            >
              {t.hero.githubCta}
            </a>
          </div>
        </section>

        {/* 2. Interactive Desktop Workbench Preview Frame */}
        <section id="showcase" className="mt-12 scroll-mt-20 sm:mt-16" aria-label={t.showcase.caption}>
          <div className="overflow-hidden rounded-2xl border border-line-strong bg-surface shadow-2xl shadow-black/60">
            <div className="flex flex-wrap items-center justify-between gap-3 border-b border-line bg-canvas/80 px-4 py-3">
              <div className="flex items-center gap-2 text-xs text-dim">
                <span className="h-2.5 w-2.5 rounded-full bg-white/15" />
                <span className="h-2.5 w-2.5 rounded-full bg-white/15" />
                <span className="h-2.5 w-2.5 rounded-full bg-white/15" />
                <span className="ml-2 font-mono text-[11px] text-muted">{t.showcase.caption}</span>
              </div>

              <div className="flex flex-wrap items-center gap-1" role="tablist">
                {slides.map((slide, idx) => {
                  const active = idx === slideIndex;
                  return (
                    <button
                      key={slide.id}
                      type="button"
                      role="tab"
                      aria-selected={active}
                      onClick={() => setSlideIndex(idx)}
                      className={`cursor-pointer rounded-lg px-2.5 py-1 text-xs font-medium transition ${
                        active
                          ? "bg-accent/15 text-accent ring-1 ring-inset ring-accent/30"
                          : "text-dim hover:bg-white/[0.05] hover:text-ink"
                      }`}
                    >
                      {slide.label}
                    </button>
                  );
                })}
              </div>
            </div>

            <div className="bg-canvas p-2 sm:p-3">
              <img
                src={assetPath(activeSlide.image)}
                alt={activeSlide.label}
                width={1360}
                height={880}
                className="mx-auto block w-full rounded-xl border border-line object-cover"
              />
            </div>

            <div className="flex flex-col items-start justify-between gap-2 border-t border-line bg-surface px-4 py-3 text-xs text-dim sm:flex-row sm:items-center">
              <span>{activeSlide.summary}</span>
              <a
                href="#download"
                className="inline-flex shrink-0 items-center gap-1 font-medium text-accent transition hover:text-accent-hover"
              >
                {t.showcase.downloadLink}
                <ArrowRight size={13} />
              </a>
            </div>
          </div>
        </section>

        {/* 3. Core Capabilities Grid */}
        <section id="features" className="scroll-mt-20 pt-20" aria-labelledby="features-title">
          <div className="max-w-2xl">
            <p className="font-mono text-xs font-semibold tracking-wider text-accent">
              {t.features.eyebrow}
            </p>
            <h2 id="features-title" className="mt-2 text-2xl font-bold tracking-tight text-ink sm:text-3xl">
              {t.features.title}
            </h2>
            <p className="mt-2.5 text-sm leading-relaxed text-dim sm:text-base">
              {t.features.subtitle}
            </p>
          </div>

          <div className="mt-8 grid gap-4 md:grid-cols-2">
            {t.features.items.map((item) => {
              const Icon = featureIcons[item.id];
              return (
                <article
                  key={item.id}
                  className="rounded-2xl border border-line bg-surface p-6 transition hover:border-line-strong"
                >
                  <div className="flex items-center justify-between gap-3">
                    <div className="flex h-10 w-10 items-center justify-center rounded-xl border border-line bg-canvas text-accent">
                      <Icon size={19} aria-hidden="true" />
                    </div>
                    <span className="font-mono text-[10px] font-semibold tracking-wider text-muted">
                      {item.badge}
                    </span>
                  </div>
                  <h3 className="mt-5 text-base font-semibold text-ink">{item.title}</h3>
                  <p className="mt-2 text-sm leading-relaxed text-dim">{item.description}</p>
                </article>
              );
            })}
          </div>
        </section>

        {/* 4. Concise 3-Step Workflow */}
        <section className="pt-20" aria-labelledby="workflow-title">
          <div className="rounded-2xl border border-line bg-surface p-6 sm:p-8">
            <p className="font-mono text-xs font-semibold tracking-wider text-accent">
              {t.workflow.eyebrow}
            </p>
            <h2 id="workflow-title" className="mt-2 text-xl font-bold tracking-tight text-ink sm:text-2xl">
              {t.workflow.title}
            </h2>

            <ol className="mt-6 grid gap-4 md:grid-cols-3">
              {t.workflow.steps.map((item) => (
                <li
                  key={item.step}
                  className="rounded-xl border border-line bg-canvas/70 p-5"
                >
                  <span className="inline-flex h-7 w-7 items-center justify-center rounded-lg bg-accent/15 font-mono text-xs font-semibold text-accent">
                    {item.step}
                  </span>
                  <h3 className="mt-4 text-sm font-semibold text-ink">{item.title}</h3>
                  <p className="mt-2 text-xs leading-relaxed text-dim">{item.description}</p>
                </li>
              ))}
            </ol>
          </div>
        </section>

        {/* 5. Download Center (Worker CDN Mirror + GitHub Direct) */}
        <Downloads />

        {/* 6. Community .smmpack Starter Packs (Cloudflare R2 Ready) */}
        <StarterPacks />

        {/* 7. FAQ Accordion */}
        <section id="faq" className="scroll-mt-20 pt-20" aria-labelledby="faq-title">
          <div className="grid gap-8 lg:grid-cols-12">
            <div className="lg:col-span-4">
              <p className="font-mono text-xs font-semibold tracking-wider text-accent">
                {t.faq.eyebrow}
              </p>
              <h2 id="faq-title" className="mt-2 text-2xl font-bold tracking-tight text-ink">
                {t.faq.title}
              </h2>
              <p className="mt-2.5 text-sm leading-relaxed text-dim">{t.faq.subtitle}</p>
            </div>

            <Accordion.Root
              type="single"
              collapsible
              className="divide-y divide-line overflow-hidden rounded-2xl border border-line bg-surface lg:col-span-8"
            >
              {t.faq.items.map((item, idx) => (
                <Accordion.Item key={idx} value={String(idx)}>
                  <Accordion.Header>
                    <Accordion.Trigger className="group flex w-full cursor-pointer items-center justify-between gap-4 px-5 py-4 text-left text-sm font-medium text-ink transition hover:bg-white/[0.02]">
                      <span>{item.q}</span>
                      <ChevronDown
                        size={16}
                        className="shrink-0 text-muted transition-transform duration-200 group-data-[state=open]:rotate-180 group-data-[state=open]:text-accent"
                        aria-hidden="true"
                      />
                    </Accordion.Trigger>
                  </Accordion.Header>
                  <Accordion.Content className="px-5 pb-4 text-sm leading-relaxed text-dim">
                    {item.a}
                  </Accordion.Content>
                </Accordion.Item>
              ))}
            </Accordion.Root>
          </div>
        </section>
      </main>

      <Footer />
    </div>
  );
}
