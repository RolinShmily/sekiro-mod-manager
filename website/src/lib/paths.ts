export const REPOSITORY = "https://github.com/RolinShmily/sekiro-mod-manager";
export function assetPath(path: string) {
  return `${process.env.NEXT_PUBLIC_BASE_PATH || ""}${path}`;
}
