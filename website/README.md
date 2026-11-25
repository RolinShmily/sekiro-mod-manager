# Sekiro Mod Manager Website

官网：<https://sekiro.srprolin.com/>。

基于 Vite + React 19 + Tailwind CSS v4 构建，配合根目录 `worker/index.ts` 与 `wrangler.toml` 部署至 Cloudflare Workers：

- 静态页面由 `website/dist` 提供服务（构建期自动生成生产依赖许可证报告并预渲染首屏 HTML）。
- `/releases/latest.json` 与 `/releases/download/vX.Y.Z/sekiro-mod-manager-vX.Y.Z-windows-x64-(setup.exe|gui.zip)` 由 Cloudflare Worker 白名单流式反代至 GitHub Releases 并开启边缘缓存。
- 社区 `.smmpack` 尝鲜整合包元数据维护于 `website/public/packs.json`，大文件托管于独立的 Cloudflare R2 公开自定义域。

## 常用命令（仓库根目录执行）

```bash
pnpm install --frozen-lockfile --ignore-scripts
pnpm dev        # 本地开发（127.0.0.1:5173）
pnpm test       # 运行 Worker 与网站解析单测
pnpm lint       # TypeScript 类型检查
pnpm build      # 许可证收集 + 客户端/SSR 构建 + 预渲染
pnpm preview    # 本地预览构建产物（127.0.0.1:4173）
```
