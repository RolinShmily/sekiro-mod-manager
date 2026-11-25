"use client";

import {
  Archive,
  ArrowDownToLine,
  ArrowUpRight,
  Check,
  Copy,
  Info,
  LoaderCircle,
  Monitor,
  RefreshCw,
} from "lucide-react";
import { useEffect, useState } from "react";
import { useI18n } from "@/components/i18n";
import { assetPath } from "@/lib/paths";
import {
  fetchJson,
  formatSize,
  parseAppRelease,
  RELEASE_API,
  RELEASE_MANIFEST_PATH,
  RELEASES_URL,
  softwareUrl,
  type AppRelease,
  type DownloadAsset,
  type DownloadSource,
} from "@/lib/downloads";

function ChecksumRow({ hash }: { hash?: string }) {
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

function ActionButton({
  asset,
  source,
  label,
  primary,
}: {
  asset?: DownloadAsset;
  source: DownloadSource;
  label: string;
  primary: boolean;
}) {
  if (!asset) {
    return (
      <span className="inline-flex h-10 cursor-not-allowed items-center justify-center gap-2 rounded-xl border border-line bg-canvas/50 px-4 text-xs font-medium text-muted">
        <ArrowDownToLine size={15} aria-hidden="true" />
        {label}
      </span>
    );
  }
  return (
    <a
      href={softwareUrl(asset.url, source)}
      rel="noreferrer"
      className={`inline-flex h-10 items-center justify-center gap-2 rounded-xl px-4 text-sm font-semibold transition ${
        primary
          ? "bg-accent text-black shadow-lg shadow-amber-500/10 hover:bg-accent-hover"
          : "border border-line-strong bg-canvas text-ink hover:border-accent/50 hover:text-accent"
      }`}
    >
      <ArrowDownToLine size={15} aria-hidden="true" />
      {label}
    </a>
  );
}

export function Downloads() {
  const { t } = useI18n();
  const [source, setSource] = useState<DownloadSource>("mirror");
  const [release, setRelease] = useState<AppRelease>();
  const [state, setState] = useState<"loading" | "ready" | "error">("loading");
  const [attempt, setAttempt] = useState(0);
  const [restored, setRestored] = useState(false);

  useEffect(() => {
    try {
      const saved = localStorage.getItem("smm_download_source");
      if (saved === "github" || saved === "mirror") setSource(saved);
    } catch {
      // Ignore storage access restrictions.
    }
    setRestored(true);
  }, []);

  useEffect(() => {
    if (!restored) return;
    const controller = new AbortController();
    setState("loading");
    setRelease(undefined);

    (async () => {
      let found: AppRelease | undefined;
      if (source === "mirror") {
        try {
          found = parseAppRelease(
            await fetchJson(assetPath(RELEASE_MANIFEST_PATH), controller.signal),
          );
        } catch {
          if (controller.signal.aborted) return;
        }
      }
      if (found) {
        setRelease(found);
        setState("ready");
        return;
      }
      try {
        found = parseAppRelease(await fetchJson(RELEASE_API, controller.signal));
        if (controller.signal.aborted) return;
        setRelease(found);
        setState(found ? "ready" : "error");
      } catch {
        if (!controller.signal.aborted) setState("error");
      }
    })();

    return () => controller.abort();
  }, [source, attempt, restored]);

  function chooseSource(next: DownloadSource) {
    setSource(next);
    try {
      localStorage.setItem("smm_download_source", next);
    } catch {
      // Ignore storage errors.
    }
  }

  return (
    <section id="download" className="scroll-mt-20 pt-20" aria-labelledby="download-title">
      <div className="max-w-2xl">
        <p className="font-mono text-xs font-semibold tracking-wider text-accent">
          {t.download.eyebrow}
        </p>
        <h2 id="download-title" className="mt-2 text-2xl font-bold tracking-tight text-ink sm:text-3xl">
          {t.download.title}
        </h2>
        <p className="mt-2.5 text-sm leading-relaxed text-dim sm:text-base">
          {t.download.subtitle}
        </p>
      </div>

      <div className="mt-8 rounded-2xl border border-line bg-surface p-6 sm:p-8">
        <div className="flex flex-col justify-between gap-4 border-b border-line pb-6 md:flex-row md:items-center">
          <div>
            <h3 className="text-lg font-bold text-ink">{t.download.appTitle}</h3>
            <p className="mt-1 text-xs leading-relaxed text-dim">{t.download.appDesc}</p>
          </div>

          <div className="flex flex-wrap items-center gap-2">
            <span className="text-xs text-muted">{t.download.sourceLabel}</span>
            <div className="inline-flex rounded-xl border border-line bg-canvas p-1">
              <button
                type="button"
                aria-pressed={source === "mirror"}
                onClick={() => chooseSource("mirror")}
                className={`cursor-pointer rounded-lg px-3 py-1.5 text-xs font-medium transition ${
                  source === "mirror"
                    ? "bg-accent/15 text-accent"
                    : "text-dim hover:text-ink"
                }`}
              >
                {t.download.sourceMirror}
              </button>
              <button
                type="button"
                aria-pressed={source === "github"}
                onClick={() => chooseSource("github")}
                className={`cursor-pointer rounded-lg px-3 py-1.5 text-xs font-medium transition ${
                  source === "github"
                    ? "bg-white/10 text-ink"
                    : "text-dim hover:text-ink"
                }`}
              >
                {t.download.sourceGithub}
              </button>
            </div>
          </div>
        </div>

        {/* Status Bar */}
        <div className="mt-4 flex flex-wrap items-center justify-between gap-3 text-xs">
          <div className="flex items-center gap-2" role="status" aria-live="polite">
            {state === "loading" ? (
              <>
                <LoaderCircle size={14} className="animate-spin text-accent" />
                <span className="text-dim">{t.download.loading}</span>
              </>
            ) : state === "ready" && release ? (
              <>
                <span className="h-2 w-2 rounded-full bg-emerald" />
                <span className="font-medium text-ink">
                  {t.download.latest} · <span className="font-mono text-emerald">{release.tag}</span>
                </span>
              </>
            ) : (
              <>
                <span className="text-amber-300">{t.download.unavailable}</span>
                <button
                  type="button"
                  onClick={() => setAttempt((v) => v + 1)}
                  className="inline-flex cursor-pointer items-center gap-1 rounded-md border border-line bg-card px-2 py-1 text-ink transition hover:border-line-strong"
                >
                  <RefreshCw size={12} />
                  {t.download.retry}
                </button>
              </>
            )}
          </div>

          <a
            href={release?.page ?? RELEASES_URL}
            target="_blank"
            rel="noreferrer"
            className="inline-flex items-center gap-1 text-dim transition hover:text-ink"
          >
            {t.download.allReleases}
            <ArrowUpRight size={13} />
          </a>
        </div>

        <p className="mt-2 text-xs text-muted">{t.download.sourceNote}</p>

        {/* Two Client Package Cards */}
        <div className="mt-6 grid gap-4 md:grid-cols-2">
          <article className="flex flex-col justify-between rounded-xl border border-line bg-card p-5">
            <div>
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2.5">
                  <div className="flex h-10 w-10 items-center justify-center rounded-xl border border-line bg-canvas text-accent">
                    <Monitor size={19} aria-hidden="true" />
                  </div>
                  <span className="rounded-md bg-accent/15 px-2 py-0.5 text-[11px] font-medium text-accent">
                    {t.download.setupBadge}
                  </span>
                </div>
                <div className="flex items-center gap-2 font-mono text-xs text-muted">
                  <span>{release ? release.tag : "Windows x64"}</span>
                  {release?.setup?.size && <span>· {formatSize(release.setup.size)}</span>}
                </div>
              </div>
              <h4 className="mt-4 text-base font-semibold text-ink">{t.download.setupTitle}</h4>
              <p className="mt-1.5 text-sm leading-relaxed text-dim">{t.download.setupDesc}</p>
            </div>

            <div className="mt-5">
              <div className="flex flex-col">
                <ActionButton
                  asset={release?.setup}
                  source={source}
                  label={`${t.download.downloadBtn} (.exe)`}
                  primary
                />
              </div>
              <ChecksumRow hash={release?.setup?.sha256} />
            </div>
          </article>

          <article className="flex flex-col justify-between rounded-xl border border-line bg-card p-5">
            <div>
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2.5">
                  <div className="flex h-10 w-10 items-center justify-center rounded-xl border border-line bg-canvas text-sky">
                    <Archive size={19} aria-hidden="true" />
                  </div>
                  <span className="rounded-md border border-line bg-canvas px-2 py-0.5 text-[11px] font-medium text-dim">
                    {t.download.portableBadge}
                  </span>
                </div>
                <div className="flex items-center gap-2 font-mono text-xs text-muted">
                  <span>{release ? release.tag : "Windows x64"}</span>
                  {release?.portable?.size && <span>· {formatSize(release.portable.size)}</span>}
                </div>
              </div>
              <h4 className="mt-4 text-base font-semibold text-ink">{t.download.portableTitle}</h4>
              <p className="mt-1.5 text-sm leading-relaxed text-dim">{t.download.portableDesc}</p>
            </div>

            <div className="mt-5">
              <div className="flex flex-col">
                <ActionButton
                  asset={release?.portable}
                  source={source}
                  label={`${t.download.downloadBtn} (.zip)`}
                  primary={false}
                />
              </div>
              <ChecksumRow hash={release?.portable?.sha256} />
            </div>
          </article>
        </div>

        {/* ModEngine compliance hint */}
        <div className="mt-6 flex flex-col items-start justify-between gap-3 rounded-xl border border-line bg-canvas/60 px-4 py-3.5 text-xs text-dim sm:flex-row sm:items-center">
          <div className="flex items-start gap-2.5">
            <Info size={15} className="mt-0.5 shrink-0 text-accent" aria-hidden="true" />
            <span className="leading-relaxed">{t.download.modEngineNote}</span>
          </div>
          <a
            href="https://www.nexusmods.com/sekiro/mods/6"
            target="_blank"
            rel="noreferrer"
            className="inline-flex shrink-0 items-center gap-1 font-medium text-accent transition hover:text-accent-hover"
          >
            {t.download.modEngineLink}
            <ArrowUpRight size={13} />
          </a>
        </div>
      </div>
    </section>
  );
}
