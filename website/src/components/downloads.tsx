"use client";

import { useEffect, useRef, useState } from "react";
import { ArrowRight, ArrowUpRight, Copy, Download, Layers, Terminal, Check } from "lucide-react";
import { useI18n } from "./i18n";
import { assetPath, REPOSITORY } from "@/lib/paths";
import { fetchRelease, formatSize, RELEASES_URL, type ReleaseLookup, type AssetKind } from "@/lib/downloads";

export function Downloads() {
  const { locale, t } = useI18n();
  const [release, setRelease] = useState<ReleaseLookup | null>(null);
  const [status, setStatus] = useState<"loading" | "ready" | "error">("loading");
  const [refresh, setRefresh] = useState(0);
  const [copyState, setCopyState] = useState<"idle" | "copied" | "error">("idle");
  const copyTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  useEffect(() => {
    const abort = new AbortController();
    setStatus("loading");
    setRelease(null);
    fetchRelease(abort.signal, assetPath("/release.json"))
      .then(data => { setRelease(data); setStatus("ready"); })
      .catch(error => { if (!abort.signal.aborted) { setStatus("error"); console.warn("Could not load release metadata", error); } });
    return () => abort.abort();
  }, [refresh]);
  useEffect(() => () => { if (copyTimer.current) clearTimeout(copyTimer.current); }, []);
  async function copy() {
    try { await navigator.clipboard.writeText("smm --json list\nsmm --json plan"); setCopyState("copied"); }
    catch (error) { setCopyState("error"); console.warn("Clipboard unavailable", error); }
    if (copyTimer.current) clearTimeout(copyTimer.current);
    copyTimer.current = setTimeout(() => setCopyState("idle"), 4000);
  }
  const complete = release && ["setup", "portable", "cli"].every(kind => release.assets[kind as AssetKind]);
  const releaseLabel = status === "loading" ? t("releaseLoading") : status === "error" ? t("releaseError") : `${t(release?.source === "snapshot" ? "releaseSnapshot" : "releaseReady")} · ${release?.tag}${complete ? "" : ` · ${t("releasePartial")}`}`;
  return <section className="download section-pad" style={{ backgroundImage: `url(${assetPath("/assets/ink.webp")})` }} id="download" aria-labelledby="download-title"><div className="container">
    <div className="section-heading reveal"><p className="eyebrow">04 / READY WHEN YOU ARE</p><div className="heading-row"><h2 id="download-title">{t("downloadTitle")}</h2><p>{t("downloadDescription")}</p></div></div>
    <div className="download-app reveal">
      <div className="download-app-head"><div><span className="mini-label">DESKTOP APPLICATION</span><h3>Sekiro Mod Manager</h3><p>{t("downloadPlatform")}</p></div><div className="release-state"><span className="caption-dot" /><span id="release-status" role="status" aria-live="polite">{releaseLabel}</span><button id="release-retry" className="text-button" disabled={status === "loading"} onClick={() => setRefresh(value => value + 1)}>{t("retry")}</button></div></div>
      <div className="download-options">{(["setup", "portable"] as const).map(kind => {
        const asset = release?.assets[kind];
        return <article className="download-option" key={kind}><span className="download-type">{kind === "setup" ? "EXE" : "ZIP"}</span><div><h4>{t(kind === "setup" ? "setupTitle" : "portableTitle")}</h4><p>{t(kind === "setup" ? "setupDescription" : "portableDescription")}</p><span className="asset-meta">{formatSize(asset?.size ?? null)}</span>{asset?.digest && <span className="asset-digest" title={asset.digest}>SHA-256: {asset.digest}</span>}</div><a className={`button ${kind === "setup" ? "button-primary" : "button-outline"} asset-download`} id={`${kind}-download`} href={asset?.url} aria-disabled={!asset} tabIndex={asset ? 0 : -1}><Download className="icon" /><span>{t("downloadButton")}</span></a></article>;
      })}</div>
      <div className="download-app-footer"><span>{t("downloadSource")}</span><a href={release?.page || RELEASES_URL} target="_blank" rel="noopener noreferrer"><span>{t("allReleases")}</span><ArrowUpRight className="icon" /></a></div>
    </div>
    <div className="developer-downloads reveal">
      <article className="developer-card"><Terminal className="icon" /><p className="mini-label">COMMAND LINE</p><h3>{t("cliTitle")}</h3><p>{t("cliDescription")}</p><div className="terminal"><code><span>smm</span> --json list<br /><span>smm</span> --json plan</code><button id="copy-cli" className="icon-button" onClick={copy} aria-label={t("copyCommands")}>{copyState === "copied" ? <Check className="icon" /> : <Copy className="icon" />}</button></div><a className="text-link asset-download" id="cli-download" href={release?.assets.cli?.url} aria-disabled={!release?.assets.cli} tabIndex={release?.assets.cli ? 0 : -1}><span>{t("cliDownload")}</span><ArrowRight className="icon" /></a></article>
      <article className="developer-card" id="skill"><Layers className="icon" /><p className="mini-label">AGENT SKILL</p><h3>{t("skillTitle")}</h3><p>{t("skillDescription")}</p><ol className="skill-install"><li>{t("skillStep1")}</li><li>{t("skillStep2")}</li><li>{t("skillStep3")}</li></ol><div className="skill-links"><a className="text-link" id="skill-download" href={assetPath("/downloads/smm-cli.zip")} download><span>{t("skillDownload")}</span><ArrowRight className="icon" /></a><a className="text-link secondary-link" href={`${REPOSITORY}/tree/main/skills/smm-cli`} target="_blank" rel="noopener noreferrer">{t("skillDocs")}</a></div><p className="skill-note">{t("skillNote")}</p></article>
    </div>
    <p className="download-help"><span>{t("firstTime")}</span><a id="readme-link" href={`${REPOSITORY}/blob/main/${locale === "zh" ? "README.zh-CN.md" : "README.md"}`} target="_blank" rel="noopener noreferrer"><span>{t("readGuide")}</span><ArrowUpRight className="icon" /></a><span id="copy-status" role="status" aria-live="polite">{copyState === "copied" ? t("copied") : copyState === "error" ? t("copyError") : ""}</span></p>
  </div></section>;
}
