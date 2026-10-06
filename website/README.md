# SMM 产品展示网站

Sekiro Mod Manager 产品展示网站：介绍桌面工作台、模组管理能力、CLI 与智能体技能，并提供软件下载。

技术栈：**Next.js 16 / React 19 / TypeScript / Tailwind CSS 4 / Radix UI / Lucide / Motion / Cloudflare Workers**。App Router 静态导出，不需要 Next.js 运行服务器。依赖版本固定在 `package.json` 和 `package-lock.json`。

## 本地运行

需要 Node.js 24（测试依赖内置 TypeScript stripping）和 Python 3.9+（仅用于 Skill ZIP 打包，标准库）。

```bash
cd website
npm ci --ignore-scripts
npm run dev
```

开发地址：`http://127.0.0.1:3000`。

生产预览：

```bash
npm test
npm run lint
npm run build
npm run preview
```

打开 `http://127.0.0.1:4173/`。`build` 自动打包当前 `skills/smm-cli`，生成 `public/downloads/smm-cli.zip` 和大小 / SHA-256 元数据，再静态导出到 `website/out/`。技能包仅包含技能说明、命令参考、调用脚本和 MIT LICENSE，不包含 CLI、模组或测试数据。

## 页面

- 雾灰、墨黑、朱红与衬线标题；官方角色美术首屏、真实 APP 缩略窗、苇名城横幅与纹理分区。
- 四个真实 APP 界面：启动工作台、模组管理、预设、全局配置。中文和英文各一套，1360 × 880 捕获并压缩为 WebP。数据是隔离环境的虚构示例，不是可下载的玩法模组。
- Radix 标签页支持鼠标、键盘方向键及前后按钮；没有自动轮播。Radix 折叠问答支持键盘操作。
- 三步部署示意只改变展示内容，不执行命令、不访问本地游戏文件。
- 中英切换保存在 localStorage，更新 `html lang` 与标题。静态初始 HTML 使用中文。存储失败不阻止使用。
- Motion 只做 350ms 以内的 transform / opacity 入场；减少动态效果模式移除动画、旋转和滚动平滑。内容默认可见，JavaScript 失败仍能阅读。
- 响应式手机导航、可见焦点、Escape 关闭菜单；320、375、390、768、1440px 布局已验证。
- 页脚显示实际技术栈、MIT 许可和美术来源。美术版权独立于源码 MIT，详见 `public/ASSETS.md`。

## 软件下载

优先查询固定仓库的 GitHub Releases API；失败时读取随网站发布的 `public/release.json` 快照。快照回退会明确标为“已发布版本快照 · 实时查询不可用”，不冒充实时最新版本。CI 构建时刷新快照；本地更新可运行：

```bash
gh api repos/RolinShmily/sekiro-mod-manager/releases/latest > public/release.json
```

下载解析只接受正式、非 draft / prerelease、严格版本匹配且 URL 属于本项目的 GUI Setup EXE、GUI ZIP 和 CLI ZIP。缺失的制品不会构造下载地址。大小 / SHA-256 仅在真实元数据存在时显示。全部查询失败时禁用下载按钮，并提供重试和官方 Releases 入口。

直接链接 GitHub 资产，不自动下载、安装或执行文件。网站没有凭据、账户收集或游戏文件访问权限。

## Cloudflare Workers

仓库根 `wrangler.toml` 定义静态 Worker `sekiro-mod-manager`，资源目录 `website/out/`。缺失资源返回 404，不把 ZIP / JSON 请求回退成首页；没有服务器入口代码。

CI `website.yml` 在网站 / Skill 改动时运行测试、类型检查、构建并上传 `smm-website` 产物。`main` 推送通过验证后自动部署到 Cloudflare；Pull Request 只验证。也支持手动触发部署。使用仓库 Secrets：

- `CLOUDFLARE_ACCOUNT_ID`
- `CLOUDFLARE_API_TOKEN`（Worker 部署权限）

本地在已登录 Wrangler 的环境中部署：

```bash
# 从仓库根执行；先完成 website 的构建
npx --yes wrangler@4.147.0 deploy --config wrangler.toml
```

不在配置中写账号或令牌，也没有设置自定义域名、DNS 或覆盖其他 Worker。`workers.dev` 提供预览入口；自定义域名可在 Cloudflare 控制台后续绑定。

## 子路径

需要 GitHub Pages 式仓库路径时，在构建时设置 `NEXT_PUBLIC_BASE_PATH=/sekiro-mod-manager`。Windows Git Bash 同时设置 `MSYS_NO_PATHCONV=1`，避免前缀被转换为本地路径。图片、静态纹理、Skill 链接、favicon 和 release snapshot 都适配此前缀；preview 自动识别构建后的前缀。Cloudflare 默认部署使用根路径，不需要设置该变量。

## 验证

`npm test` 覆盖下载仓库边界、不同版本资产、预发布、缺失文件、无效 digest、API 快照回退与取消。浏览器验收覆盖中英文记忆、鼠标和键盘切页、手动前后切换、移动端菜单、真实 ZIP 下载、失败状态、重试、减少动态效果和无水合错误。

截图、捕获日志与浏览器验收结果位于仓库忽略目录 `build/verification/website/`。未操作真实游戏目录和用户 GUI 配置。
