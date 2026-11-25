"use client";

import { ArrowDownToLine, Globe2, Menu, Package, X } from "lucide-react";
import { useState } from "react";
import { useI18n } from "@/components/i18n";
import { assetPath, REPOSITORY } from "@/lib/paths";

function GithubMark() {
  return (
    <svg width="17" height="17" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
      <path d="M12 .5A11.5 11.5 0 0 0 .5 12c0 5.08 3.29 9.39 7.86 10.91.58.11.79-.25.79-.56 0-.28-.01-1.02-.02-2-3.2.7-3.87-1.54-3.87-1.54-.52-1.33-1.28-1.69-1.28-1.69-1.05-.71.08-.7.08-.7 1.16.08 1.77 1.19 1.77 1.19 1.03 1.76 2.7 1.25 3.36.96.1-.75.4-1.25.73-1.54-2.55-.29-5.24-1.28-5.24-5.69 0-1.26.45-2.28 1.19-3.09-.12-.29-.52-1.46.11-3.05 0 0 .97-.31 3.17 1.18a10.97 10.97 0 0 1 5.77 0c2.2-1.49 3.17-1.18 3.17-1.18.63 1.59.23 2.76.11 3.05.74.81 1.19 1.83 1.19 3.09 0 4.42-2.69 5.4-5.26 5.68.42.36.79 1.06.79 2.13 0 1.54-.01 2.78-.01 3.16 0 .31.21.67.8.56A11.5 11.5 0 0 0 23.5 12 11.5 11.5 0 0 0 12 .5Z" />
    </svg>
  );
}

export function Navbar() {
  const { locale, toggleLocale, t } = useI18n();
  const [mobileOpen, setMobileOpen] = useState(false);

  const navItems = [
    { href: "#showcase", label: t.nav.showcase },
    { href: "#features", label: t.nav.features },
    { href: "#download", label: t.nav.download, icon: ArrowDownToLine },
    { href: "#packs", label: t.nav.packs, icon: Package },
    { href: "#faq", label: t.nav.faq },
  ];

  return (
    <header className="sticky top-0 z-40 border-b border-line bg-canvas/85 backdrop-blur-xl">
      <a
        href="#main"
        className="sr-only-focusable fixed left-4 top-3 z-50 rounded-lg bg-accent px-3 py-1.5 text-xs font-semibold text-black"
      >
        {t.nav.skip}
      </a>

      <div className="mx-auto flex h-16 max-w-6xl items-center justify-between gap-4 px-4 sm:px-6">
        <div className="flex min-w-0 items-center gap-6">
          <a
            href="#top"
            className="flex shrink-0 items-center gap-2.5 whitespace-nowrap text-sm font-semibold tracking-tight text-ink"
          >
            <img
              src={assetPath("/assets/icon-128.png")}
              alt=""
              width="28"
              height="28"
              className="shrink-0 rounded-lg"
            />
            <span>
              Sekiro Mod Manager<span className="text-accent">.</span>
            </span>
          </a>

          <nav className="hidden items-center gap-1 md:flex" aria-label="Primary">
            {navItems.map(({ href, label, icon: Icon }) => (
              <a
                key={href}
                href={href}
                className="inline-flex shrink-0 items-center gap-1.5 whitespace-nowrap rounded-lg px-3 py-1.5 text-sm font-medium text-dim transition hover:bg-white/[0.05] hover:text-ink"
              >
                {Icon && <Icon size={14} className="text-muted" aria-hidden="true" />}
                {label}
              </a>
            ))}
          </nav>
        </div>

        <div className="flex shrink-0 items-center gap-2">
          <button
            type="button"
            onClick={toggleLocale}
            aria-label={t.nav.language}
            title={t.nav.language}
            className="inline-flex h-9 shrink-0 cursor-pointer items-center gap-1.5 whitespace-nowrap rounded-lg border border-line bg-surface px-2.5 text-xs font-medium text-dim transition hover:border-line-strong hover:text-ink"
          >
            <Globe2 size={14} aria-hidden="true" />
            <span>{locale === "zh" ? "EN" : "中"}</span>
          </button>

          <a
            href={REPOSITORY}
            target="_blank"
            rel="noreferrer"
            aria-label="GitHub"
            className="inline-flex h-9 w-9 items-center justify-center rounded-lg border border-line bg-surface text-dim transition hover:border-line-strong hover:text-ink"
          >
            <GithubMark />
          </a>

          <button
            type="button"
            onClick={() => setMobileOpen((prev) => !prev)}
            aria-expanded={mobileOpen}
            aria-label={mobileOpen ? t.nav.close : t.nav.menu}
            className="inline-flex h-9 w-9 cursor-pointer items-center justify-center rounded-lg border border-line bg-surface text-dim transition hover:text-ink md:hidden"
          >
            {mobileOpen ? <X size={17} /> : <Menu size={17} />}
          </button>
        </div>
      </div>

      {mobileOpen && (
        <nav aria-label="Mobile" className="border-t border-line bg-surface px-4 py-3 md:hidden">
          <div className="flex flex-col gap-1">
            {navItems.map(({ href, label, icon: Icon }) => (
              <a
                key={href}
                href={href}
                onClick={() => setMobileOpen(false)}
                className="flex items-center gap-2 rounded-lg px-3 py-2 text-sm font-medium text-dim transition hover:bg-white/[0.05] hover:text-ink"
              >
                {Icon && <Icon size={15} className="text-muted" aria-hidden="true" />}
                {label}
              </a>
            ))}
          </div>
        </nav>
      )}
    </header>
  );
}
