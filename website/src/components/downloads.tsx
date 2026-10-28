"use client";

import { useEffect, useRef, useState } from "react";
import {
  ArrowRight,
  ArrowUpRight,
  Copy,
  Download,
  Terminal,
  Check,
  CheckCircle2,
  Clock,
  AlertCircle,
  Bot,
  BookOpen,
  HelpCircle,
  Shield,
  FileCode,
} from "lucide-react";
import { useI18n } from "./i18n";
import { assetPath, REPOSITORY } from "@/lib/paths";
import {
  fetchRelease,
  formatSize,
  RELEASES_URL,
  type ReleaseLookup,
  type AssetKind,
} from "@/lib/downloads";
import { WindowsIcon, ExeBadgeIcon, ZipBadgeIcon } from "./icons";

export function Downloads() {
  const { locale, t } = useI18n();
  const [release, setRelease] = useState<ReleaseLookup | null>(null);
  const [status, setStatus] = useState<"loading" | "ready" | "error">("loading");
  const [refresh, setRefresh] = useState(0);
  const [copyState, setCopyState] = useState<"idle" | "copied" | "error">("idle");
  const [copiedSha, setCopiedSha] = useState<string | null>(null);
  const copyTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const shaTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    const abort = new AbortController();
    setStatus("loading");
    setRelease(null);
    fetchRelease(abort.signal, assetPath("/release.json"))
      .then((data) => {
        setRelease(data);
        setStatus("ready");
      })
      .catch((error) => {
        if (!abort.signal.aborted) {
          setStatus("error");
          console.warn("Could not load release metadata", error);
        }
      });
    return () => abort.abort();
  }, [refresh]);

  useEffect(() => {
    return () => {
      if (copyTimer.current) clearTimeout(copyTimer.current);
      if (shaTimer.current) clearTimeout(shaTimer.current);
    };
  }, []);

  async function copy() {
    try {
      await navigator.clipboard.writeText("smm --json list\nsmm --json plan");
      setCopyState("copied");
    } catch (error) {
      setCopyState("error");
      console.warn("Clipboard unavailable", error);
    }
    if (copyTimer.current) clearTimeout(copyTimer.current);
    copyTimer.current = setTimeout(() => setCopyState("idle"), 4000);
  }

  async function copySha(digest: string) {
    try {
      await navigator.clipboard.writeText(digest);
      setCopiedSha(digest);
      if (shaTimer.current) clearTimeout(shaTimer.current);
      shaTimer.current = setTimeout(() => setCopiedSha(null), 3000);
    } catch (error) {
      console.warn("Could not copy SHA", error);
    }
  }

  const complete =
    release && ["setup", "portable", "cli"].every((kind) => release.assets[kind as AssetKind]);
  const releaseLabel =
    status === "loading"
      ? t("releaseLoading")
      : status === "error"
      ? t("releaseError")
      : `${t(release?.source === "snapshot" ? "releaseSnapshot" : "releaseReady")} · ${release?.tag}${
          complete ? "" : ` · ${t("releasePartial")}`
        }`;

  return (
    <section
      className="download section-pad"
      style={{ backgroundImage: `url(${assetPath("/assets/ink.webp")})` }}
      id="download"
      aria-labelledby="download-title"
    >
      <div className="container">
        <div className="section-heading reveal">
          <p className="eyebrow">04 / READY WHEN YOU ARE</p>
          <div className="heading-row">
            <h2 id="download-title">{t("downloadTitle")}</h2>
            <p>{t("downloadDescription")}</p>
          </div>
        </div>
        <div className="download-app reveal">
          <div className="download-app-head">
            <div>
              <span className="mini-label flex items-center gap-1.5">
                <WindowsIcon className="w-3.5 h-3.5 text-gold inline" />
                DESKTOP APPLICATION
              </span>
              <h3>Sekiro Mod Manager</h3>
              <p className="flex items-center gap-2 mt-1 text-on-night-muted">
                <span>{t("downloadPlatform")}</span>
              </p>
            </div>
            <div className="release-state">
              {status === "loading" ? (
                <Clock size={14} className="text-gold animate-spin" />
              ) : status === "ready" ? (
                <CheckCircle2 size={14} className="text-[#68d391]" />
              ) : (
                <AlertCircle size={14} className="text-[#f56565]" />
              )}
              <span id="release-status" role="status" aria-live="polite">
                {releaseLabel}
              </span>
              <button
                id="release-retry"
                className="text-button"
                disabled={status === "loading"}
                onClick={() => setRefresh((value) => value + 1)}
              >
                {t("retry")}
              </button>
            </div>
          </div>
          <div className="download-options">
            {(["setup", "portable"] as const).map((kind) => {
              const asset = release?.assets[kind];
              return (
                <article className="download-option" key={kind}>
                  <div className="download-type-box">
                    {kind === "setup" ? (
                      <ExeBadgeIcon className="w-6 h-6 text-gold" />
                    ) : (
                      <ZipBadgeIcon className="w-6 h-6 text-gold" />
                    )}
                    <span className="download-type-name">{kind === "setup" ? "SETUP" : "ZIP"}</span>
                  </div>
                  <div>
                    <h4>{t(kind === "setup" ? "setupTitle" : "portableTitle")}</h4>
                    <p>{t(kind === "setup" ? "setupDescription" : "portableDescription")}</p>
                    <span className="asset-meta flex items-center gap-1.5">
                      <WindowsIcon className="w-3 h-3 text-gold opacity-75" />
                      {formatSize(asset?.size ?? null)}
                    </span>
                    {asset?.digest && (
                      <div className="asset-digest-row">
                        <span className="asset-digest" title={asset.digest}>
                          SHA-256: {asset.digest}
                        </span>
                        <button
                          className="digest-copy-btn"
                          onClick={() => copySha(asset.digest!)}
                          aria-label="Copy SHA-256"
                          title="Copy SHA-256"
                        >
                          {copiedSha === asset.digest ? <Check size={12} /> : <Copy size={12} />}
                        </button>
                      </div>
                    )}
                  </div>
                  <a
                    className={`button ${
                      kind === "setup" ? "button-primary" : "button-outline"
                    } asset-download`}
                    id={`${kind}-download`}
                    href={asset?.url}
                    aria-disabled={!asset}
                    tabIndex={asset ? 0 : -1}
                  >
                    <Download className="icon" />
                    <span>{t("downloadButton")}</span>
                  </a>
                </article>
              );
            })}
          </div>
          <div className="download-app-footer">
            <span className="flex items-center gap-1.5">
              <Shield size={13} className="text-gold" />
              {t("downloadSource")}
            </span>
            <a href={release?.page || RELEASES_URL} target="_blank" rel="noopener noreferrer">
              <span>{t("allReleases")}</span>
              <ArrowUpRight className="icon" />
            </a>
          </div>
        </div>
        <div className="developer-downloads reveal">
          <article className="developer-card">
            <Terminal className="icon" />
            <p className="mini-label">COMMAND LINE</p>
            <h3>{t("cliTitle")}</h3>
            <p>{t("cliDescription")}</p>
            <div className="terminal">
              <code>
                <span>smm</span> --json list
                <br />
                <span>smm</span> --json plan
              </code>
              <button
                id="copy-cli"
                className="icon-button"
                onClick={copy}
                aria-label={t("copyCommands")}
                title={t("copyCommands")}
              >
                {copyState === "copied" ? <Check className="icon" /> : <Copy className="icon" />}
              </button>
            </div>
            <a
              className="text-link asset-download"
              id="cli-download"
              href={release?.assets.cli?.url}
              aria-disabled={!release?.assets.cli}
              tabIndex={release?.assets.cli ? 0 : -1}
            >
              <FileCode className="icon" />
              <span>{t("cliDownload")}</span>
              <ArrowRight className="icon ml-1" />
            </a>
          </article>
          <article className="developer-card" id="skill">
            <Bot className="icon" />
            <p className="mini-label">AGENT SKILL</p>
            <h3>{t("skillTitle")}</h3>
            <p>{t("skillDescription")}</p>
            <ol className="skill-install">
              <li>
                <span className="skill-step-num">1</span>
                <span>{t("skillStep1")}</span>
              </li>
              <li>
                <span className="skill-step-num">2</span>
                <span>{t("skillStep2")}</span>
              </li>
              <li>
                <span className="skill-step-num">3</span>
                <span>{t("skillStep3")}</span>
              </li>
            </ol>
            <div className="skill-links">
              <a
                className="text-link"
                id="skill-download"
                href={assetPath("/downloads/smm-cli.zip")}
                download
              >
                <Download className="icon" />
                <span>{t("skillDownload")}</span>
                <ArrowRight className="icon ml-1" />
              </a>
              <a
                className="text-link secondary-link"
                href={`${REPOSITORY}/tree/main/skills/smm-cli`}
                target="_blank"
                rel="noopener noreferrer"
              >
                <BookOpen size={14} className="mr-1" />
                {t("skillDocs")}
              </a>
            </div>
            <p className="skill-note">{t("skillNote")}</p>
          </article>
        </div>
        <p className="download-help">
          <HelpCircle size={15} className="text-gold" />
          <span>{t("firstTime")}</span>
          <a
            id="readme-link"
            href={`${REPOSITORY}/blob/main/${locale === "zh" ? "README.zh-CN.md" : "README.md"}`}
            target="_blank"
            rel="noopener noreferrer"
          >
            <span>{t("readGuide")}</span>
            <ArrowUpRight className="icon" />
          </a>
          <span id="copy-status" role="status" aria-live="polite">
            {copyState === "copied"
              ? t("copied")
              : copyState === "error"
              ? t("copyError")
              : copiedSha
              ? (locale === "zh" ? "SHA-256 已复制到剪贴板。" : "SHA-256 copied to clipboard.")
              : ""}
          </span>
        </p>
      </div>
    </section>
  );
}
