export const RELEASES_URL = "https://github.com/RolinShmily/sekiro-mod-manager/releases";
export const RELEASE_API = "https://api.github.com/repos/RolinShmily/sekiro-mod-manager/releases/latest";
const DOWNLOAD_ROOT = "https://github.com/RolinShmily/sekiro-mod-manager/releases/download/";
export type AssetKind = "setup" | "portable" | "cli";
export type ReleaseAsset = { name: string; url: string; size: number | null; digest: string | null };
export type StableRelease = { tag: string; page: string; assets: Partial<Record<AssetKind, ReleaseAsset>> };

function object(value: unknown): Record<string, unknown> | null {
  return typeof value === "object" && value !== null && !Array.isArray(value) ? value as Record<string, unknown> : null;
}

export function parseRelease(payload: unknown): StableRelease | null {
  const release = object(payload);
  if (!release || release.draft !== false || release.prerelease !== false || typeof release.tag_name !== "string") return null;
  const tag = release.tag_name;
  if (!/^v\d+\.\d+\.\d+$/.test(tag) || release.html_url !== `${RELEASES_URL}/tag/${tag}` || !Array.isArray(release.assets)) return null;
  const result: StableRelease = { tag, page: release.html_url, assets: {} };
  const names: Record<AssetKind, string> = {
    setup: `sekiro-mod-manager-${tag}-windows-x64-gui-setup.exe`,
    portable: `sekiro-mod-manager-${tag}-windows-x64-gui.zip`,
    cli: `sekiro-mod-manager-${tag}-windows-x64-cli.zip`,
  };
  for (const item of release.assets) {
    const asset = object(item);
    if (!asset || asset.state !== "uploaded") continue;
    for (const kind of Object.keys(names) as AssetKind[]) {
      const name = names[kind];
      if (asset.name !== name || asset.browser_download_url !== `${DOWNLOAD_ROOT}${tag}/${name}`) continue;
      result.assets[kind] = {
        name, url: asset.browser_download_url,
        size: typeof asset.size === "number" && Number.isFinite(asset.size) && asset.size > 0 ? asset.size : null,
        digest: typeof asset.digest === "string" && /^sha256:[a-f\d]{64}$/i.test(asset.digest) ? asset.digest.slice(7).toLowerCase() : null,
      };
    }
  }
  return result;
}

export type ReleaseLookup = StableRelease & { source: "live" | "snapshot" };
export async function fetchRelease(signal: AbortSignal, snapshotUrl: string): Promise<ReleaseLookup> {
  let lastError: unknown;
  for (const url of [RELEASE_API, snapshotUrl]) {
    if (signal.aborted) throw new DOMException("Request aborted", "AbortError");
    try {
      const response = await fetch(url, { signal: AbortSignal.any([signal, AbortSignal.timeout(6000)]), cache: "no-store", headers: { Accept: "application/json" } });
      if (!response.ok) throw new Error(`Release HTTP ${response.status}`);
      const release = parseRelease(await response.json());
      if (!release) throw new Error("No valid stable release in response");
      return { ...release, source: url === RELEASE_API ? "live" : "snapshot" };
    } catch (error) {
      if (signal.aborted) throw error;
      lastError = error;
    }
  }
  throw new Error("Release lookup failed", { cause: lastError });
}

export function formatSize(size: number | null): string {
  if (size === null) return "Windows x64";
  const megabytes = size / (1024 * 1024);
  return `${megabytes.toFixed(1)} MB · Windows x64`;
}
