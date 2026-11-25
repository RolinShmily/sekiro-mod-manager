import test from "node:test";
import assert from "node:assert/strict";
import {
  RELEASES_URL,
  SITE_ORIGIN,
  formatSize,
  packDownloadUrl,
  parseAppRelease,
  parsePacksManifest,
  softwareUrl,
} from "../../website/src/lib/downloads.ts";

const GITHUB_SETUP =
  `${RELEASES_URL}/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-setup.exe`;
const GITHUB_GUI =
  `${RELEASES_URL}/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-gui.zip`;
const MIRROR_SETUP =
  `${SITE_ORIGIN}/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-setup.exe`;
const MIRROR_GUI =
  `${SITE_ORIGIN}/releases/download/v0.4.8/sekiro-mod-manager-v0.4.8-windows-x64-gui.zip`;

test("softwareUrl routes cleanly between Worker CDN mirror and GitHub direct", () => {
  assert.equal(softwareUrl(GITHUB_SETUP, "mirror"), MIRROR_SETUP);
  assert.equal(softwareUrl(MIRROR_SETUP, "github"), GITHUB_SETUP);
  assert.equal(softwareUrl(GITHUB_GUI, "mirror"), MIRROR_GUI);
  assert.equal(softwareUrl(MIRROR_GUI, "github"), GITHUB_GUI);
  assert.equal(softwareUrl("https://example.com/other.zip", "mirror"), "https://example.com/other.zip");
});

test("parseAppRelease parses Worker latest.json manifest", () => {
  const parsed = parseAppRelease({
    version: "0.4.8",
    tag: "v0.4.8",
    setup_exe_url: MIRROR_SETUP,
    setup_exe_sha256: "a".repeat(64),
    setup_exe_size: 16105313,
    gui_zip_url: MIRROR_GUI,
    gui_zip_sha256: "b".repeat(64),
    gui_zip_size: 21515939,
  });
  assert.ok(parsed);
  assert.equal(parsed.version, "0.4.8");
  assert.equal(parsed.tag, "v0.4.8");
  assert.equal(parsed.setup?.url, MIRROR_SETUP);
  assert.equal(parsed.setup?.sha256, "a".repeat(64));
  assert.equal(parsed.setup?.size, 16105313);
  assert.equal(parsed.portable?.url, MIRROR_GUI);
  assert.equal(parsed.portable?.sha256, "b".repeat(64));
});

test("parseAppRelease parses GitHub Releases API payload and ignores drafts/prereleases", () => {
  const apiPayload = {
    tag_name: "v0.4.8",
    draft: false,
    prerelease: false,
    assets: [
      {
        name: "sekiro-mod-manager-v0.4.8-windows-x64-setup.exe",
        state: "uploaded",
        size: 16105313,
        digest: "sha256:" + "c".repeat(64),
        browser_download_url: GITHUB_SETUP,
      },
      {
        name: "sekiro-mod-manager-v0.4.8-windows-x64-gui.zip",
        state: "uploaded",
        size: 21515939,
        digest: "sha256:" + "d".repeat(64),
        browser_download_url: GITHUB_GUI,
      },
    ],
  };
  const parsed = parseAppRelease(apiPayload);
  assert.ok(parsed);
  assert.equal(parsed.setup?.url, GITHUB_SETUP);
  assert.equal(parsed.setup?.sha256, "c".repeat(64));
  assert.equal(parsed.portable?.url, GITHUB_GUI);
  assert.equal(parsed.portable?.sha256, "d".repeat(64));

  assert.equal(parseAppRelease({ ...apiPayload, prerelease: true }), undefined);
  assert.equal(parseAppRelease({ ...apiPayload, draft: true }), undefined);
  assert.equal(
    parseAppRelease({ version: "0.4.8", setup_exe_url: "https://evil.example/a-windows-x64-setup.exe" }),
    undefined,
  );
});

test("parsePacksManifest parses dual-domain R2 starter packs and included Nexus mod links", () => {
  const hosted = parsePacksManifest({
    schema_version: 1,
    cdn_base_url: "https://cdn-r2-sekiro.srprolin.com/",
    origin_base_url: "https://r2-sekiro.srprolin.com/",
    packs: [
      {
        id: "miyamoto-musashi",
        file: "宫本武藏.smmpack",
        object_key: "packs/miyamoto-musashi.smmpack",
        size_bytes: 208348972,
        sha256: "e".repeat(64),
        title_zh: "宫本武藏",
        title_en: "Miyamoto Musashi",
        summary_zh: "简介",
        summary_en: "Summary",
        mods: [
          { name: "Miyamoto Musashi", source_url: "https://www.nexusmods.com/sekiro/mods/1264" },
          { name: "Unsafe Link", source_url: "javascript:alert(1)" },
        ],
      },
    ],
  });
  assert.equal(hosted.length, 1);
  assert.equal(
    packDownloadUrl(hosted[0], "cdn"),
    "https://cdn-r2-sekiro.srprolin.com/packs/miyamoto-musashi.smmpack",
  );
  assert.equal(
    packDownloadUrl(hosted[0], "origin"),
    "https://r2-sekiro.srprolin.com/packs/miyamoto-musashi.smmpack",
  );
  assert.equal(hosted[0].sha256, "e".repeat(64));
  assert.equal(formatSize(hosted[0].sizeBytes), "198.7 MB");
  assert.equal(hosted[0].mods.length, 1);
  assert.equal(hosted[0].mods[0].name, "Miyamoto Musashi");
});
