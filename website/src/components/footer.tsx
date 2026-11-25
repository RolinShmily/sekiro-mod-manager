"use client";

import { ArrowUpRight, Scale } from "lucide-react";
import { useI18n } from "@/components/i18n";
import { assetPath, REPOSITORY } from "@/lib/paths";
import { RELEASES_URL } from "@/lib/downloads";

export function Footer() {
  const { t } = useI18n();

  return (
    <footer className="border-t border-line bg-surface/60">
      <div className="mx-auto max-w-6xl px-4 py-10 sm:px-6">
        <div className="flex flex-col justify-between gap-8 sm:flex-row sm:items-start">
          <div className="max-w-sm space-y-2.5">
            <a
              href="#top"
              className="inline-flex items-center gap-2.5 text-sm font-semibold text-ink"
            >
              <img
                src={assetPath("/assets/icon-128.png")}
                alt=""
                width="24"
                height="24"
                className="rounded-md"
              />
              <span>
                Sekiro Mod Manager<span className="text-accent">.</span>
              </span>
            </a>
            <p className="text-xs leading-relaxed text-dim">{t.footer.description}</p>
          </div>

          <div className="flex flex-wrap gap-x-10 gap-y-6 text-xs">
            <div className="space-y-2">
              <div className="font-semibold uppercase tracking-wider text-muted">
                {t.footer.siteHeading}
              </div>
              <ul className="space-y-1.5 text-dim">
                <li><a href="#showcase" className="transition hover:text-ink">{t.nav.showcase}</a></li>
                <li><a href="#features" className="transition hover:text-ink">{t.nav.features}</a></li>
                <li><a href="#download" className="transition hover:text-ink">{t.nav.download}</a></li>
                <li><a href="#packs" className="transition hover:text-ink">{t.nav.packs}</a></li>
                <li><a href="#faq" className="transition hover:text-ink">{t.nav.faq}</a></li>
              </ul>
            </div>

            <div className="space-y-2">
              <div className="font-semibold uppercase tracking-wider text-muted">
                {t.footer.projectHeading}
              </div>
              <ul className="space-y-1.5 text-dim">
                <li>
                  <a
                    href={REPOSITORY}
                    target="_blank"
                    rel="noreferrer"
                    className="inline-flex items-center gap-1 transition hover:text-ink"
                  >
                    {t.footer.project} <ArrowUpRight size={12} />
                  </a>
                </li>
                <li>
                  <a
                    href={RELEASES_URL}
                    target="_blank"
                    rel="noreferrer"
                    className="inline-flex items-center gap-1 transition hover:text-ink"
                  >
                    {t.footer.releases} <ArrowUpRight size={12} />
                  </a>
                </li>
                <li>
                  <a
                    href={`${REPOSITORY}/tree/main/docs`}
                    target="_blank"
                    rel="noreferrer"
                    className="inline-flex items-center gap-1 transition hover:text-ink"
                  >
                    {t.footer.docs} <ArrowUpRight size={12} />
                  </a>
                </li>
                <li>
                  <a
                    href="https://blog.srprolin.com"
                    target="_blank"
                    rel="noreferrer"
                    className="inline-flex items-center gap-1 transition hover:text-ink"
                  >
                    {t.footer.blog} <ArrowUpRight size={12} />
                  </a>
                </li>
              </ul>
            </div>
          </div>
        </div>

        <div className="mt-8 flex flex-col items-start justify-between gap-3 border-t border-line pt-6 text-xs text-muted sm:flex-row sm:items-center">
          <span>© 2025–2026 RoL1n_SrP · {t.footer.disclaimer}</span>
          <div className="flex flex-wrap items-center gap-4">
            <a
              href={`${REPOSITORY}/blob/main/LICENSE`}
              target="_blank"
              rel="noreferrer"
              className="inline-flex items-center gap-1 text-dim transition hover:text-ink"
            >
              <Scale size={13} aria-hidden="true" />
              {t.footer.license}
            </a>
            <a
              href={`${REPOSITORY}/blob/main/THIRD_PARTY_NOTICES`}
              target="_blank"
              rel="noreferrer"
              className="text-dim transition hover:text-ink"
            >
              {t.footer.notices}
            </a>
            <a
              href={assetPath("/third-party-licenses.txt")}
              target="_blank"
              rel="noreferrer"
              className="text-dim transition hover:text-ink"
            >
              {t.footer.webLicenses}
            </a>
          </div>
        </div>
      </div>
    </footer>
  );
}
