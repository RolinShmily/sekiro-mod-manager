import { fileURLToPath } from "node:url";
import react from "@vitejs/plugin-react";
import { defineConfig, loadEnv, type Plugin } from "vite";
import { SITE_URL } from "./src/lib/paths.ts";

/** Accepts "/", "docs", "/docs/" and normalizes to a leading and trailing slash. */
function normalizeBase(value: string) {
  if (/^https?:\/\//.test(value)) return `${value.replace(/\/+$/, "")}/`;
  return `/${value.replace(/^\/+/, "").replace(/\/+$/, "")}/`.replace("//", "/");
}

/**
 * index.html is a static file, so the deployment prefix is filled in at build
 * time. Absolute asset URLs and canonical / Open Graph URLs both need it.
 */
function siteRoot(base: string): Plugin {
  const prefix = base.replace(/\/+$/, "");
  return {
    name: "smm-site-root",
    transformIndexHtml(html) {
      return html.replaceAll("%SITE_URL%", SITE_URL).replaceAll("%ASSET_BASE%", prefix);
    },
  };
}

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), "");
  const base = normalizeBase(env.VITE_BASE_PATH || "/");
  return {
    base,
    plugins: [react(), siteRoot(base)],
    resolve: {
      alias: { "@": fileURLToPath(new URL("./src", import.meta.url)) },
    },
    build: {
      // Bundles go to /static so the /assets folder stays reserved for images.
      assetsDir: "static",
      target: "es2022",
    },
    server: { host: "127.0.0.1", port: 5173 },
    preview: { host: "127.0.0.1", port: 4173 },
  };
});
