export const SITE_URL = "https://sekiro.srprolin.com";
export const REPOSITORY = "https://github.com/RolinShmily/sekiro-mod-manager";

type ViteBase = { env?: { BASE_URL?: string } };

/** Resolves a root-relative URL against the deployment prefix (Vite `base`). */
export function assetPath(path: string) {
  const base = (import.meta as ViteBase).env?.BASE_URL ?? "/";
  return `${base.replace(/\/+$/, "")}${path}`;
}
