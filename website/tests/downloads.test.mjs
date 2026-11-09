import assert from "node:assert/strict";
import { test } from "node:test";
import { parseRelease, formatSize, fetchRelease, RELEASE_API, MIRROR, softwareUrl } from "../src/lib/downloads.ts";

const repository = "https://github.com/RolinShmily/sekiro-mod-manager/releases";
function asset(tag, kind) {
  const suffix = { setup: "setup.exe", portable: "gui.zip", cli: "cli.zip" }[kind];
  const name = `sekiro-mod-manager-${tag}-windows-x64-${suffix}`;
  return { name, state: "uploaded", size: 10485760, digest: "sha256:" + "a".repeat(64), browser_download_url: `${repository}/download/${tag}/${name}` };
}
function fixture() { return { tag_name: "v1.2.3", draft: false, prerelease: false, html_url: `${repository}/tag/v1.2.3`, assets: [asset("v1.2.3", "setup"), asset("v1.2.3", "portable"), asset("v1.2.3", "cli")] }; }

test("separates GUI installer, portable and CLI from a real stable release shape", () => {
  const parsed = parseRelease(fixture());
  assert.equal(parsed.tag, "v1.2.3");
  assert.equal(parsed.assets.setup.name.endsWith("windows-x64-setup.exe"), true);
  assert.equal(parsed.assets.portable.name.endsWith("gui.zip"), true);
  assert.equal(parsed.assets.cli.name.endsWith("cli.zip"), true);
  assert.equal(parsed.assets.portable.digest, "a".repeat(64));
  assert.equal(formatSize(parsed.assets.setup.size), "10.0 MB · Windows x64");
  assert.equal(parsed.assets.setup.url, asset("v1.2.3", "setup").browser_download_url);
});
test("softwareUrl prefixes with MIRROR by default and supports direct GitHub and env var override", () => {
  const url = "https://github.com/RolinShmily/sekiro-mod-manager/releases/download/v1.0.0/app.exe";
  assert.equal(softwareUrl(url), "https://gh-proxy.org/" + url);
  assert.equal(softwareUrl(url, "mirror"), "https://gh-proxy.org/" + url);
  assert.equal(softwareUrl(url, "github"), url);

  assert.equal(softwareUrl(url, "mirror", "https://custom.mirror/"), "https://custom.mirror/" + url);

  const prev = process.env.VITE_DOWNLOAD_MIRROR;
  try {
    process.env.VITE_DOWNLOAD_MIRROR = "https://env-mirror.org/";
    assert.equal(softwareUrl(url, "mirror"), "https://env-mirror.org/" + url);
    process.env.VITE_DOWNLOAD_MIRROR = "none";
    assert.equal(softwareUrl(url, "mirror"), url);
  } finally {
    if (prev === undefined) delete process.env.VITE_DOWNLOAD_MIRROR;
    else process.env.VITE_DOWNLOAD_MIRROR = prev;
  }
});
test("rejects draft/prerelease and malformed metadata rather than fabricating links", () => {
  for (const payload of [null, {}, [], { ...fixture(), draft: true }, { ...fixture(), prerelease: true }, { ...fixture(), tag_name: "v1.2.3-rc1" }, { ...fixture(), html_url: "https://example.com/release" }]) assert.equal(parseRelease(payload), null);
});
test("rejects asset URL substitution, different versions and unuploaded files", () => {
  for (const altered of [{ browser_download_url: "https://example.com/app.exe" }, { browser_download_url: "https://github.com/other/repo/releases/download/v1.2.3/app.exe" }, { browser_download_url: asset("v9.9.9", "setup").browser_download_url }, { state: "new" }, { name: asset("v9.9.9", "setup").name }]) {
    const payload = fixture();
    payload.assets[0] = { ...payload.assets[0], ...altered };
    assert.equal(parseRelease(payload).assets.setup, undefined);
  }
});
test("missing artifacts remain missing; invalid digest or size is not displayed", () => {
  const payload = fixture();
  payload.assets = [{ ...payload.assets[1], digest: "not-a-sha", size: -1 }];
  const result = parseRelease(payload);
  assert.equal(result.assets.setup, undefined);
  assert.equal(result.assets.cli, undefined);
  assert.equal(result.assets.portable.size, null);
  assert.equal(result.assets.portable.digest, null);
});
test("API failure falls back to the published same-site snapshot", async () => {
  const previousFetch = globalThis.fetch;
  const calls = [];
  globalThis.fetch = async url => { calls.push(url); if (url === RELEASE_API) return new Response("limited", { status: 403 }); return Response.json(fixture()); };
  try { const result = await fetchRelease(new AbortController().signal, "/release.json"); assert.equal(result.tag, "v1.2.3"); assert.deepEqual(calls, [RELEASE_API, "/release.json"]); }
  finally { globalThis.fetch = previousFetch; }
});
test("all sources failing raises an error and cancellation does not fall back", async () => {
  const previousFetch = globalThis.fetch;
  globalThis.fetch = async () => new Response("error", { status: 503 });
  try { await assert.rejects(fetchRelease(new AbortController().signal, "/release.json")); const abort = new AbortController(); abort.abort(); await assert.rejects(fetchRelease(abort.signal, "/release.json"), error => error.name === "AbortError"); }
  finally { globalThis.fetch = previousFetch; }
});
