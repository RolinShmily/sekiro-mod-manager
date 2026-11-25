export const SITE_ORIGIN = "https://sekiro.srprolin.com";
export const REPOSITORY = "https://github.com/RolinShmily/sekiro-mod-manager";
export const RELEASES_URL = `${REPOSITORY}/releases`;
export const RELEASE_MANIFEST_PATH = "/releases/latest.json";
export const RELEASE_API =
  "https://api.github.com/repos/RolinShmily/sekiro-mod-manager/releases/latest";

export type DownloadSource = "mirror" | "github";
export type PackSource = "cdn" | "origin";

export interface DownloadAsset {
  url: string;
  sha256?: string;
  size?: number;
}

export interface AppRelease {
  version: string;
  tag: string;
  page: string;
  setup?: DownloadAsset;
  portable?: DownloadAsset;
}

export interface PackIncludedMod {
  name: string;
  sourceUrl: string;
}

export interface StarterPack {
  id: string;
  file: string;
  cdnUrl?: string;
  originUrl?: string;
  sizeBytes?: number;
  sha256?: string;
  updatedAt?: string;
  titleZh: string;
  titleEn: string;
  summaryZh: string;
  summaryEn: string;
  tagsZh: string[];
  tagsEn: string[];
  mods: PackIncludedMod[];
}

const GITHUB_DOWNLOAD_PREFIX = "/RolinShmily/sekiro-mod-manager/releases/download/";
const SITE_DOWNLOAD_PREFIX = "/releases/download/";
const SEMVER_RE = /^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$/;
const SHA256_RE = /^[a-f0-9]{64}$/i;

function object(value: unknown): Record<string, unknown> | undefined {
  return value !== null && typeof value === "object" && !Array.isArray(value)
    ? (value as Record<string, unknown>)
    : undefined;
}

function checksum(value: unknown): string | undefined {
  return typeof value === "string" && SHA256_RE.test(value) ? value.toLowerCase() : undefined;
}

function positiveInt(value: unknown): number | undefined {
  return typeof value === "number" && Number.isSafeInteger(value) && value > 0 ? value : undefined;
}

function extractReleaseSuffix(url: string): string | undefined {
  try {
    const parsed = new URL(url);
    if (parsed.search || parsed.hash || parsed.username || parsed.password) return undefined;
    if (parsed.origin === "https://github.com" && parsed.pathname.startsWith(GITHUB_DOWNLOAD_PREFIX)) {
      return parsed.pathname.slice(GITHUB_DOWNLOAD_PREFIX.length);
    }
    if (parsed.origin === SITE_ORIGIN && parsed.pathname.startsWith(SITE_DOWNLOAD_PREFIX)) {
      return parsed.pathname.slice(SITE_DOWNLOAD_PREFIX.length);
    }
    return undefined;
  } catch (error) {
    if (error instanceof TypeError) return undefined;
    throw error;
  }
}

function isReleaseAssetUrl(url: unknown, tag: string, suffix: string): url is string {
  if (typeof url !== "string") return false;
  const extracted = extractReleaseSuffix(url);
  return Boolean(extracted && extracted.startsWith(`${tag}/`) && extracted.endsWith(suffix));
}

export function softwareUrl(url: string, source: DownloadSource = "mirror"): string {
  const suffix = extractReleaseSuffix(url);
  if (!suffix) return url;
  return source === "mirror"
    ? `${SITE_ORIGIN}${SITE_DOWNLOAD_PREFIX}${suffix}`
    : `${RELEASES_URL}/download/${suffix}`;
}

export function packDownloadUrl(pack: StarterPack, source: PackSource = "cdn"): string | undefined {
  return source === "cdn" ? (pack.cdnUrl ?? pack.originUrl) : (pack.originUrl ?? pack.cdnUrl);
}

export function parseAppRelease(value: unknown): AppRelease | undefined {
  const obj = object(value);
  if (!obj) return undefined;
  const rawVersion = obj.version ?? obj.tag_name;
  if (typeof rawVersion !== "string" || !SEMVER_RE.test(rawVersion)) return undefined;
  if (obj.prerelease === true || obj.draft === true) return undefined;

  const version = rawVersion.replace(/^v/, "");
  const tag = `v${version}`;
  if (obj.tag !== undefined && obj.tag !== tag) return undefined;

  const result: AppRelease = {
    version,
    tag,
    page: `${RELEASES_URL}/tag/${tag}`,
  };

  if (isReleaseAssetUrl(obj.setup_exe_url, tag, "-windows-x64-setup.exe")) {
    result.setup = {
      url: obj.setup_exe_url,
      sha256: checksum(obj.setup_exe_sha256),
      size: positiveInt(obj.setup_exe_size),
    };
  }
  const guiUrl = obj.gui_zip_url ?? obj.url;
  if (isReleaseAssetUrl(guiUrl, tag, "-windows-x64-gui.zip")) {
    result.portable = {
      url: guiUrl,
      sha256: checksum(obj.gui_zip_sha256 ?? obj.sha256),
      size: positiveInt(obj.gui_zip_size),
    };
  }

  if (Array.isArray(obj.assets)) {
    for (const item of obj.assets) {
      const asset = object(item);
      if (!asset) continue;
      if (asset.state !== undefined && asset.state !== "uploaded") continue;
      const sha =
        typeof asset.digest === "string"
          ? checksum(asset.digest.replace(/^sha256:/, ""))
          : undefined;
      if (isReleaseAssetUrl(asset.browser_download_url, tag, "-windows-x64-setup.exe")) {
        result.setup = {
          url: asset.browser_download_url,
          size: positiveInt(asset.size),
          sha256: sha,
        };
      }
      if (isReleaseAssetUrl(asset.browser_download_url, tag, "-windows-x64-gui.zip")) {
        result.portable = {
          url: asset.browser_download_url,
          size: positiveInt(asset.size),
          sha256: sha,
        };
      }
    }
  }

  return result.setup || result.portable ? result : undefined;
}

function isSafeHttpsUrl(raw: unknown): raw is string {
  if (typeof raw !== "string" || !raw.trim()) return false;
  try {
    const parsed = new URL(raw);
    return parsed.protocol === "https:" && !parsed.username && !parsed.password;
  } catch {
    return false;
  }
}

function buildObjectUrl(baseUrl: string, objectKey: string): string {
  const cleanKey = objectKey
    .replace(/^\/+/, "")
    .split("/")
    .map((segment) => encodeURIComponent(segment))
    .join("/");
  return `${baseUrl}/${cleanKey}`;
}

export function parsePacksManifest(value: unknown): StarterPack[] {
  const obj = object(value);
  if (!obj || obj.schema_version !== 1 || !Array.isArray(obj.packs)) return [];

  const cdnBase = isSafeHttpsUrl(obj.cdn_base_url)
    ? obj.cdn_base_url.replace(/\/+$/, "")
    : "";
  const originBase = isSafeHttpsUrl(obj.origin_base_url ?? obj.r2_base_url)
    ? String(obj.origin_base_url ?? obj.r2_base_url).replace(/\/+$/, "")
    : "";

  const packs: StarterPack[] = [];
  for (const item of obj.packs) {
    const entry = object(item);
    if (!entry) continue;
    if (
      typeof entry.id !== "string" ||
      typeof entry.file !== "string" ||
      typeof entry.title_zh !== "string" ||
      typeof entry.title_en !== "string" ||
      typeof entry.summary_zh !== "string" ||
      typeof entry.summary_en !== "string"
    ) {
      continue;
    }

    const objectKey =
      typeof entry.object_key === "string" && entry.object_key.trim()
        ? entry.object_key.trim()
        : entry.file;

    const cdnUrl = isSafeHttpsUrl(entry.cdn_url)
      ? entry.cdn_url
      : cdnBase
        ? buildObjectUrl(cdnBase, objectKey)
        : undefined;
    const originUrl = isSafeHttpsUrl(entry.origin_url ?? entry.url)
      ? String(entry.origin_url ?? entry.url)
      : originBase
        ? buildObjectUrl(originBase, objectKey)
        : undefined;

    const mods: PackIncludedMod[] = [];
    if (Array.isArray(entry.mods)) {
      for (const m of entry.mods) {
        const modObj = object(m);
        if (modObj && typeof modObj.name === "string" && isSafeHttpsUrl(modObj.source_url)) {
          mods.push({ name: modObj.name, sourceUrl: modObj.source_url });
        }
      }
    }

    packs.push({
      id: entry.id,
      file: entry.file,
      cdnUrl,
      originUrl,
      sizeBytes: positiveInt(entry.size_bytes),
      sha256: checksum(entry.sha256),
      updatedAt: typeof entry.updated_at === "string" ? entry.updated_at : undefined,
      titleZh: entry.title_zh,
      titleEn: entry.title_en,
      summaryZh: entry.summary_zh,
      summaryEn: entry.summary_en,
      tagsZh: Array.isArray(entry.tags_zh)
        ? entry.tags_zh.filter((v): v is string => typeof v === "string")
        : [],
      tagsEn: Array.isArray(entry.tags_en)
        ? entry.tags_en.filter((v): v is string => typeof v === "string")
        : [],
      mods,
    });
  }
  return packs;
}

export async function fetchJson(url: string, signal?: AbortSignal): Promise<unknown> {
  const response = await fetch(url, {
    signal: signal
      ? AbortSignal.any([signal, AbortSignal.timeout(9000)])
      : AbortSignal.timeout(9000),
    cache: "no-store",
    headers: { Accept: "application/json" },
  });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  const text = await response.text();
  if (text.length > 512 * 1024) throw new Error("Manifest exceeds size limit");
  return JSON.parse(text.replace(/^\uFEFF/, "")) as unknown;
}

export function formatSize(bytes?: number): string {
  if (!bytes) return "";
  if (bytes < 1024 * 1024) return `${Math.max(1, Math.round(bytes / 1024))} KB`;
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}
