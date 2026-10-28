"use client";

import { useEffect, useState } from "react";
import { motion, useMotionValueEvent, useScroll } from "motion/react";
import { ArrowDownToLine, Globe2, Menu, Terminal, X } from "lucide-react";
import { useReducedEffects } from "@/lib/use-reduced-effects";
import { useI18n } from "./i18n";
import { REPOSITORY } from "@/lib/paths";
import { BrandLogo, GithubIcon } from "./icons";

export function Brand() {
  return (
    <a href="#top" className="nav-brand" aria-label="Sekiro Mod Manager">
      <BrandLogo className="w-7 h-7 rounded-md shrink-0" />
      <span>Sekiro SMM</span>
      <span className="brand-dot" aria-hidden="true" />
    </a>
  );
}

export function Navbar() {
  const { locale, setLocale, t } = useI18n();
  const [compact, setCompact] = useState(false);
  const [open, setOpen] = useState(false);

  useEffect(() => {
    if (!open) return;
    const close = (event: KeyboardEvent) => {
      if (event.key === "Escape") setOpen(false);
    };
    document.addEventListener("keydown", close);
    return () => document.removeEventListener("keydown", close);
  }, [open]);

  const reduce = useReducedEffects();
  const { scrollY } = useScroll();
  useMotionValueEvent(scrollY, "change", (value) => setCompact(value > 90));

  const links = [
    { href: "#showcase", label: t("navShowcase") },
    { href: "#features", label: t("navFeatures") },
    { href: "#workflow", label: t("navWorkflow") },
    { href: "#download", label: t("navDownload") },
  ];

  return (
    <header className={`site-header ${compact ? "is-compact" : ""}`} data-compact={compact}>
      <a className="skip-link" href="#main">
        {t("skip")}
      </a>
      <motion.div
        className="nav-islands"
        layout
        transition={{
          type: "spring",
          stiffness: 160,
          damping: 26,
          duration: reduce ? 0 : undefined,
        }}
      >
        <a href="#top" className="nav-brand" aria-label="Sekiro Mod Manager">
          <BrandLogo className="w-7 h-7 rounded-md shrink-0" />
          <span>Sekiro SMM</span>
          <span className="brand-dot" aria-hidden="true" />
        </a>

        <nav className="nav-links" aria-label={t("navigation")}>
          {links.map((link) => (
            <a href={link.href} key={link.href}>
              {link.label}
            </a>
          ))}
        </nav>

        <div className="nav-actions">
          <button
            onClick={() => setLocale(locale === "zh" ? "en" : "zh")}
            aria-label={locale === "zh" ? "Switch to English" : "切换到简体中文"}
            title={locale === "zh" ? "Switch to English" : "切换到简体中文"}
          >
            <Globe2 size={16} />
            <span>{locale === "zh" ? "EN" : "中"}</span>
          </button>
          <a
            className="github-link"
            href={REPOSITORY}
            target="_blank"
            rel="noreferrer"
            aria-label="GitHub"
          >
            <GithubIcon className="w-4 h-4" />
          </a>
          <a
            className="nav-skill"
            href="#skill"
            aria-label="Skill"
            title="Skill"
            onClick={() => setOpen(false)}
          >
            <Terminal size={15} aria-hidden="true" />
            <span>Skill</span>
          </a>
          <a className="nav-download" href="#download" aria-label={t("navDownload")}>
            <ArrowDownToLine size={15} />
            <span>{t("navDownload")}</span>
          </a>
          <button
            className="nav-menu"
            aria-expanded={open}
            aria-controls="mobile-nav"
            aria-label={open ? t("menuOpen") : t("menuOpen")}
            onClick={() => setOpen(!open)}
          >
            {open ? <X size={19} /> : <Menu size={19} />}
          </button>
        </div>
      </motion.div>

      {open && (
        <nav id="mobile-nav" className="mobile-nav" aria-label={t("navigation")}>
          {links.map((link) => (
            <a href={link.href} key={link.href} onClick={() => setOpen(false)}>
              {link.label}
            </a>
          ))}
          <a
            href={REPOSITORY}
            target="_blank"
            rel="noreferrer"
            className="flex items-center gap-2 px-3 py-2"
            onClick={() => setOpen(false)}
          >
            <GithubIcon className="w-4 h-4" />
            <span>GitHub ↗</span>
          </a>
        </nav>
      )}
    </header>
  );
}
