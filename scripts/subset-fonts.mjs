// Sekiro Mod Manager — Font subsetting build script
// 按量构建：仅提取源码实际渲染的中文字形，生成微缩 woff2。
// 输入：node_modules/@fontsource/*（锁版本，pnpm-lock 保证可复现）
// 输出：apps/smm-desktop/public/fonts/*.woff2（产物全部瘦身，无全量字体进入安装包）
//
// 运行：pnpm fonts:subset （作为 desktop:build 前置步骤自动执行）
import subsetFont from 'subset-font';
import { readFileSync, writeFileSync, readdirSync, mkdirSync, copyFileSync, rmSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const SRC_DIRS = [
  join(ROOT, 'apps/smm-desktop/src'),
  join(ROOT, 'apps/smm-desktop'),
  join(ROOT, 'crates'),
];
const OUT_DIR = join(ROOT, 'apps/smm-desktop/public/fonts');
const FS = (p) => join(ROOT, 'node_modules/@fontsource', p);

const NOTO = {
  400: FS('noto-sans-sc/files/noto-sans-sc-chinese-simplified-400-normal.woff2'),
  600: FS('noto-sans-sc/files/noto-sans-sc-chinese-simplified-600-normal.woff2'),
  700: FS('noto-sans-sc/files/noto-sans-sc-chinese-simplified-700-normal.woff2'),
};
const LATIN_COPY = [
  ['inter', 400], ['inter', 500], ['inter', 600], ['inter', 700],
  ['jetbrains-mono', 400], ['jetbrains-mono', 700],
];

// ---------- 1) 扫描源码收集实际用到的字符 ----------
const files = [];
function walk(dir) {
  for (const e of readdirSync(dir, { withFileTypes: true })) {
    const p = join(dir, e.name);
    if (e.isDirectory()) {
      if (['node_modules', 'dist', 'target', 'src-tauri', 'public'].includes(e.name)) continue;
      walk(p);
    } else if (/\.(tsx|ts|css|html|md|rs)$/.test(e.name)) files.push(p);
  }
}
for (const d of SRC_DIRS) if (exists(d)) walk(d);

function exists(p) { try { readdirSync(p); return true; } catch { return false; } }

const chars = new Set();
// ASCII 全集（等宽兜底场景会用到；体积可忽略）
for (let i = 0x20; i <= 0x7E; i++) chars.add(String.fromCharCode(i));
for (const f of files) {
  const txt = readFileSync(f, 'utf8');
  for (const ch of txt) {
    const cp = ch.codePointAt(0);
    // CJK 统一表意 / 扩展A / CJK 标点 / 全角 / 中文常用通用标点
    if ((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x3000 && cp <= 0x303F) || (cp >= 0xFF00 && cp <= 0xFFEF) ||
        [0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x2026, 0x00B7].includes(cp)) chars.add(ch);
  }
}
const text = [...chars].join('');
console.log(`[subset] scanned ${files.length} files, ${text.length} unique chars`);

// ---------- 2) 中文按量子集 ----------
mkdirSync(OUT_DIR, { recursive: true });
for (const f of readdirSync(OUT_DIR)) rmSync(join(OUT_DIR, f), { force: true });

for (const [weight, src] of Object.entries(NOTO)) {
  const t = Date.now();
  const buf = await subsetFont(readFileSync(src), text, { targetFormat: 'woff2' });
  const out = join(OUT_DIR, `noto-sans-sc-${weight}.woff2`);
  writeFileSync(out, buf);
  console.log(`[subset] noto-sans-sc ${weight}: ${(buf.length / 1024).toFixed(1)} KB (${Date.now() - t}ms)`);
}

// ---------- 3) 拉丁字体直接复制 fontsource 的 latin 子集 ----------
for (const [family, weight] of LATIN_COPY) {
  const src = FS(`${family}/files/${family}-latin-${weight}-normal.woff2`);
  copyFileSync(src, join(OUT_DIR, `${family}-${weight}.woff2`));
}
console.log(`[subset] copied ${LATIN_COPY.length} latin subsets (Inter/JetBrains Mono)`);
console.log(`[subset] done -> ${OUT_DIR}`);