"use client";

import { useEffect, useRef, useState } from "react";
import { Code, Menu, X, ArrowUpRight } from "lucide-react";
import { useI18n } from "./i18n";
import { REPOSITORY } from "@/lib/paths";

export function Brand() {
  return <a className="brand" href="#top" aria-label="Sekiro Mod Manager"><span className="brand-seal" aria-hidden="true">隻<br />狼</span><span className="brand-wordmark">SEKIRO <span>MOD MANAGER</span></span></a>;
}
export function Navbar() {
  const { locale, setLocale, t } = useI18n();
  const [open, setOpen] = useState(false);
  const menu = useRef<HTMLButtonElement>(null);
  const navigation = [{ id: "showcase", key: "navShowcase" }, { id: "features", key: "navFeatures" }, { id: "workflow", key: "navWorkflow" }, { id: "download", key: "navDownload" }] as const;
  useEffect(() => {
    function close(event: KeyboardEvent) { if (event.key === "Escape" && open) { setOpen(false); menu.current?.focus(); } }
    document.addEventListener("keydown", close);
    return () => document.removeEventListener("keydown", close);
  }, [open]);
  return <header className="site-header" id="site-header">
    <Brand />
    <nav className="desktop-nav" aria-label={t("navigation")}>{navigation.map(item => <a key={item.id} href={`#${item.id}`}>{t(item.key)}</a>)}</nav>
    <div className="header-actions">
      <button className="language-toggle" id="language-toggle" onClick={() => setLocale(locale === "zh" ? "en" : "zh")} aria-label={locale === "zh" ? "Switch to English" : "切换到简体中文"}>{locale === "zh" ? "EN" : "中文"}<ArrowUpRight size={12} className="ml-2 inline-block" /></button>
      <a className="github-link" href={REPOSITORY} target="_blank" rel="noopener noreferrer" aria-label="GitHub"><Code className="icon" /><span>GitHub</span></a>
      <button ref={menu} className="menu-toggle" id="menu-toggle" aria-expanded={open} aria-controls="mobile-nav" aria-label={t("menuOpen")} onClick={() => setOpen(value => !value)}>{open ? <X className="icon" /> : <Menu className="icon" />}</button>
    </div>
    <nav id="mobile-nav" className="mobile-nav" hidden={!open} aria-label={t("navigation")}>{navigation.map(item => <a key={item.id} href={`#${item.id}`} onClick={() => setOpen(false)}>{t(item.key)}</a>)}<a href={REPOSITORY} target="_blank" rel="noopener noreferrer">GitHub ↗</a></nav>
  </header>;
}
