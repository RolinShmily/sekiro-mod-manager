import { createServer } from "node:http";
import { readFile, stat } from "node:fs/promises";
import { resolve, relative, extname } from "node:path";

const root = resolve(import.meta.dirname, "../out");
const port = Number(process.env.PORT || 4173);
let prefix = "";
try {
  const html = await readFile(resolve(root, "index.html"), "utf8");
  prefix = html.match(/src="([^"]*)\/_next\/static\//)?.[1] || "";
} catch (error) {
  if (error.code === "ENOENT") { console.error("Run npm run build before preview."); process.exit(1); }
  throw error;
}
const types = { ".html": "text/html; charset=utf-8", ".css": "text/css; charset=utf-8", ".js": "text/javascript; charset=utf-8", ".json": "application/json", ".svg": "image/svg+xml", ".webp": "image/webp", ".zip": "application/zip", ".md": "text/plain; charset=utf-8", ".ico": "image/x-icon", ".txt": "text/plain; charset=utf-8" };
const server = createServer(async (request, response) => {
  if (request.method !== "GET" && request.method !== "HEAD") { response.writeHead(405); response.end(); return; }
  try {
    const pathname = decodeURIComponent(new URL(request.url, "http://localhost").pathname);
    if (prefix && pathname !== prefix && !pathname.startsWith(prefix + "/")) { response.writeHead(404); response.end("Not found"); return; }
    const local = pathname.slice(prefix.length) || "/";
    let file = resolve(root, "." + local);
    const normalized = relative(root, file);
    if (normalized.startsWith("..") || normalized.includes(":")) { response.writeHead(403); response.end(); return; }
    const info = await stat(file);
    if (info.isDirectory()) file = resolve(file, "index.html");
    const data = await readFile(file);
    response.writeHead(200, { "Content-Type": types[extname(file)] || "application/octet-stream", "Cache-Control": "no-store", "X-Content-Type-Options": "nosniff" });
    response.end(request.method === "HEAD" ? undefined : data);
  } catch (error) {
    if (error instanceof URIError) { response.writeHead(400); response.end("Invalid path"); }
    else if (error.code === "ENOENT" || error.code === "ENOTDIR") { response.writeHead(404); response.end("Not found"); }
    else { console.error("Preview request failed", error); response.writeHead(500); response.end("Preview error"); }
  }
});
server.listen(port, "127.0.0.1", () => console.log(`SMM preview: http://127.0.0.1:${port}${prefix}/`));
