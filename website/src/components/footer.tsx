"use client";

import { ArrowUp, ArrowUpRight, BookOpen, GitFork, PackageOpen, Rss, Scale } from "lucide-react";
import { useI18n } from "@/components/i18n";
import { REPOSITORY } from "@/lib/paths";
import { RELEASES_URL } from "@/lib/downloads";
import { BrandLogo } from "./icons";

export function Footer() {
  const { locale, t } = useI18n();

  const links = [
    {
      label: locale === "zh" ? "项目源码" : "Repository",
      url: REPOSITORY,
      icon: GitFork,
      highlight: true,
    },
    {
      label: locale === "zh" ? "使用说明" : "Documentation",
      url: `${REPOSITORY}#readme`,
      icon: BookOpen,
      highlight: false,
    },
    {
      label: locale === "zh" ? "发布记录" : "Releases",
      url: RELEASES_URL,
      icon: PackageOpen,
      highlight: false,
    },
    {
      label: locale === "zh" ? "作者博客" : "Blog",
      url: "https://blog.srprolin.com",
      icon: Rss,
      highlight: false,
    },
  ];

  return (
    <footer className="site-footer">
      <div className="container">
        <div className="footer-main">
          <div className="footer-identity">
            <a href="#top" className="footer-brand">
              <BrandLogo className="w-10 h-10 rounded-lg shrink-0" />
              <span>
                Sekiro-SMM<span className="brand-dot-accent">.</span>
              </span>
            </a>
            <p>
              {locale === "zh"
                ? "为《只狼》玩家打造的模组工作台。配置清晰，顺位自由。"
                : "A native modding workspace for Sekiro: Shadows Die Twice. Clear priorities, total freedom."}
            </p>
          </div>
          <nav className="footer-links" aria-label={locale === "zh" ? "项目链接" : "Project links"}>
            {links.map(({ label, url, icon: Icon, highlight }) => (
              <a
                href={url}
                key={url}
                target="_blank"
                rel="noreferrer"
                className={highlight ? "footer-link-highlight" : ""}
              >
                <Icon size={18} aria-hidden="true" />
                <span>{label}</span>
                <ArrowUpRight size={14} aria-hidden="true" />
              </a>
            ))}
          </nav>
        </div>
        <div className="footer-bottom">
          <span>© 2024–2026 RoL1n_SrP</span>
          <a
            className="footer-license"
            href={`${REPOSITORY}/blob/main/LICENSE`}
            target="_blank"
            rel="noreferrer"
          >
            <Scale size={16} aria-hidden="true" />
            <span>MIT License</span>
            <ArrowUpRight size={13} aria-hidden="true" />
          </a>
          <a href="#top" className="footer-top">
            <span>{t("backTop")}</span>
            <ArrowUp size={16} aria-hidden="true" />
          </a>
        </div>
      </div>
    </footer>
  );
}
