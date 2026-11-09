/**
 * Injects the prerendered markup into dist/index.html so the landing copy is
 * present in the served HTML (SEO, share previews, no-JS first paint).
 *
 * A failing prerender must not break the deployable client-only build: the
 * shell stays valid and the app still hydrates from scratch, so we warn.
 */
import { readFile, rm, writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import { pathToFileURL } from "node:url";

const root = resolve(import.meta.dirname, "..");
const indexFile = resolve(root, "dist/index.html");
const ssrEntry = resolve(root, ".ssr/entry-server.js");
const marker = '<div id="root"></div>';

const shell = await readFile(indexFile, "utf8");
if (!shell.includes(marker)) {
  console.error(`Prerender skipped: ${indexFile} has no empty #root placeholder.`);
  process.exit(1);
}

try {
  const { render } = await import(pathToFileURL(ssrEntry).href);
  const markup = render();
  if (!markup.trim()) throw new Error("Prerender produced empty markup");
  await writeFile(indexFile, shell.replace(marker, `<div id="root">${markup}</div>`), "utf8");
  console.log(`Prerendered ${markup.length} characters into dist/index.html`);
} catch (error) {
  console.warn("Prerender failed; shipping the client-only shell instead.", error);
} finally {
  await rm(resolve(root, ".ssr"), { recursive: true, force: true });
}
