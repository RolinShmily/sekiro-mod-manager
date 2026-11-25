"use client";

import {
  ArrowDownToLine,
  ArrowUpRight,
  Check,
  Clock,
  Copy,
  Layers,
  Package,
  ShieldAlert,
} from "lucide-react";
import { useEffect, useState } from "react";
import { useI18n } from "@/components/i18n";
import { assetPath, REPOSITORY } from "@/lib/paths";
import {
  fetchJson,
  formatSize,
  packDownloadUrl,
  parsePacksManifest,
  type PackSource,
  type StarterPack,
} from "@/lib/downloads";
import bundledPacks from "../../public/packs.json";

const DEFAULT_PACKS: StarterPack[] = parsePacksManifest(bundledPacks);

function PackHashRow({ hash }: { hash?: string }) {
  const { t } = useI18n();
  const [status, setStatus] = useState<"idle" | "copied" | "failed">("idle");

  useEffect(() => {
    if (status === "idle") return;
    const timer = setTimeout(() => setStatus("idle"), 2600);
    return () => clearTimeout(timer);
  }, [status]);

  if (!hash) return null;

  async function copy() {
    if (!hash) return;
    try {
      await navigator.clipboard.writeText(hash);
      setStatus("copied");
    } catch {
      setStatus("failed");
    }
  }

  return (
    <details className="mt-3 rounded-lg border border-line bg-canvas/70 px-3 py-2 text-xs">
      <summary className="cursor-pointer select-none font-mono text-[11px] text-dim transition hover:text-ink">
        SHA-256
      </summary>
      <div className="mt-2 space-y-2">
        <code className="block break-all font-mono text-[11px] text-dim">{hash}</code>
        <button
          type="button"
          onClick={copy}
          className="inline-flex cursor-pointer items-center gap-1.5 rounded-md border border-line bg-surface px-2.5 py-1 text-[11px] font-medium text-ink transition hover:border-line-strong"
        >
          {status === "copied" ? <Check size={12} className="text-emerald" /> : <Copy size={12} />}
          <span>
            {status === "copied"
              ? t.download.hashCopied
              : status === "failed"
                ? t.download.hashFailed
                : t.download.hashCopy}
          </span>
        </button>
      </div>
    </details>
  );
}

export function StarterPacks() {
  const { locale, t } = useI18n();
  const [packs, setPacks] = useState<StarterPack[]>(DEFAULT_PACKS);
  const [source, setSource] = useState<PackSource>("cdn");

  useEffect(() => {
    try {
      const saved = localStorage.getItem("smm_pack_source");
      if (saved === "cdn" || saved === "origin") setSource(saved);
    } catch {
      // Ignore storage restrictions.
    }
  }, []);

  useEffect(() => {
    const controller = new AbortController();
    fetchJson(assetPath("/packs.json"), controller.signal)
      .then((payload) => {
        if (controller.signal.aborted) return;
        const parsed = parsePacksManifest(payload);
        if (parsed.length > 0) setPacks(parsed);
      })
      .catch(() => {
        // Keep bundled fallback cards when manifest cannot be fetched.
      });
    return () => controller.abort();
  }, []);

  function chooseSource(next: PackSource) {
    setSource(next);
    try {
      localStorage.setItem("smm_pack_source", next);
    } catch {
      // Ignore storage errors.
    }
  }

  return (
    <section id="packs" className="scroll-mt-20 pt-20" aria-labelledby="packs-title">
      <div className="flex flex-col justify-between gap-4 md:flex-row md:items-end">
        <div className="max-w-2xl">
          <p className="font-mono text-xs font-semibold tracking-wider text-accent">
            {t.packs.eyebrow}
          </p>
          <h2 id="packs-title" className="mt-2 text-2xl font-bold tracking-tight text-ink sm:text-3xl">
            {t.packs.title}
          </h2>
          <p className="mt-2.5 text-sm leading-relaxed text-dim sm:text-base">
            {t.packs.subtitle}
          </p>
        </div>

        <div className="flex flex-wrap items-center gap-2 self-start md:self-end">
          <span className="text-xs text-muted">{t.packs.routeLabel}</span>
          <div className="inline-flex rounded-xl border border-line bg-surface p-1">
            <button
              type="button"
              aria-pressed={source === "cdn"}
              onClick={() => chooseSource("cdn")}
              className={`cursor-pointer rounded-lg px-3 py-1.5 text-xs font-medium transition ${
                source === "cdn" ? "bg-accent/15 text-accent" : "text-dim hover:text-ink"
              }`}
            >
              {t.packs.routeCdn}
            </button>
            <button
              type="button"
              aria-pressed={source === "origin"}
              onClick={() => chooseSource("origin")}
              className={`cursor-pointer rounded-lg px-3 py-1.5 text-xs font-medium transition ${
                source === "origin" ? "bg-white/10 text-ink" : "text-dim hover:text-ink"
              }`}
            >
              {t.packs.routeOrigin}
            </button>
          </div>
        </div>
      </div>

      {/* Prominent Copyright & Non-Affiliation Disclaimer */}
      <div className="mt-6 rounded-2xl border border-accent/25 bg-accent/[0.05] p-5">
        <div className="flex items-start gap-3">
          <ShieldAlert size={18} className="mt-0.5 shrink-0 text-accent" aria-hidden="true" />
          <div className="space-y-1.5 text-xs leading-relaxed">
            <h3 className="font-semibold text-ink">{t.packs.disclaimerTitle}</h3>
            <p className="text-dim">{t.packs.disclaimerBody}</p>
            <div className="pt-1">
              <a
                href={`${REPOSITORY}/issues/new`}
                target="_blank"
                rel="noreferrer"
                className="inline-flex items-center gap-1 font-medium text-accent transition hover:text-accent-hover"
              >
                {t.packs.takedownLink}
                <ArrowUpRight size={13} />
              </a>
            </div>
          </div>
        </div>
      </div>

      {/* Three Starter .smmpack Cards */}
      <div className="mt-6 grid gap-4 md:grid-cols-3">
        {packs.map((pack) => {
          const title = locale === "zh" ? pack.titleZh : pack.titleEn;
          const summary = locale === "zh" ? pack.summaryZh : pack.summaryEn;
          const tags = locale === "zh" ? pack.tagsZh : pack.tagsEn;
          const href = packDownloadUrl(pack, source);

          return (
            <article
              key={pack.id}
              data-pack={pack.id}
              className="flex flex-col justify-between rounded-2xl border border-line bg-surface p-5 transition hover:border-line-strong"
            >
              <div>
                <div className="flex items-center justify-between gap-2">
                  <div className="flex h-9 w-9 items-center justify-center rounded-xl border border-line bg-canvas text-accent">
                    <Package size={17} aria-hidden="true" />
                  </div>
                  <div className="flex items-center gap-1.5 font-mono text-xs text-muted">
                    {pack.sizeBytes && <span>{formatSize(pack.sizeBytes)}</span>}
                    {pack.updatedAt && <span>· {pack.updatedAt}</span>}
                  </div>
                </div>

                <h3 className="mt-4 text-base font-semibold text-ink">{title}</h3>
                <p className="mt-1 font-mono text-[11px] text-muted">{pack.file}</p>
                <p className="mt-3 text-sm leading-relaxed text-dim">{summary}</p>

                {tags.length > 0 && (
                  <div className="mt-4 flex flex-wrap gap-1.5">
                    {tags.map((tag) => (
                      <span
                        key={tag}
                        className="rounded-md border border-line bg-canvas px-2 py-0.5 text-[11px] text-dim"
                      >
                        {tag}
                      </span>
                    ))}
                  </div>
                )}

                {pack.mods.length > 0 && (
                  <details className="mt-4 rounded-lg border border-line bg-canvas/60 px-3 py-2 text-xs">
                    <summary className="flex cursor-pointer select-none items-center gap-1.5 font-medium text-dim transition hover:text-ink">
                      <Layers size={13} className="text-accent" aria-hidden="true" />
                      <span>
                        {t.packs.includedMods} ({pack.mods.length})
                      </span>
                    </summary>
                    <ul className="mt-2 space-y-1.5 border-t border-line pt-2 text-[11px]">
                      {pack.mods.map((mod) => (
                        <li key={`${pack.id}-${mod.name}`} className="flex items-center justify-between gap-2">
                          <span className="truncate text-dim">{mod.name}</span>
                          <a
                            href={mod.sourceUrl}
                            target="_blank"
                            rel="noreferrer"
                            className="inline-flex shrink-0 items-center gap-0.5 font-mono text-[10px] text-accent transition hover:text-accent-hover"
                          >
                            Nexus
                            <ArrowUpRight size={11} />
                          </a>
                        </li>
                      ))}
                    </ul>
                  </details>
                )}
              </div>

              <div className="mt-5">
                {href ? (
                  <a
                    href={href}
                    download={pack.file}
                    rel="noreferrer"
                    className="inline-flex h-10 w-full items-center justify-center gap-2 rounded-xl border border-line-strong bg-canvas px-4 text-sm font-semibold text-ink transition hover:border-accent/50 hover:text-accent"
                  >
                    <ArrowDownToLine size={15} aria-hidden="true" />
                    {t.packs.downloadPack}
                  </a>
                ) : (
                  <span
                    title={t.packs.pendingNote}
                    className="inline-flex h-10 w-full cursor-not-allowed items-center justify-center gap-2 rounded-xl border border-line bg-canvas/50 px-4 text-xs font-medium text-muted"
                  >
                    <Clock size={14} aria-hidden="true" />
                    {t.packs.pendingBtn}
                  </span>
                )}
                <PackHashRow hash={pack.sha256} />
              </div>
            </article>
          );
        })}
      </div>

      <p className="mt-4 text-xs text-muted">{t.packs.readyHint}</p>
    </section>
  );
}
