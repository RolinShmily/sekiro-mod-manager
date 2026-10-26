import type { Metadata, Viewport } from "next";
import { I18nProvider } from "@/components/i18n";
import { assetPath, SITE_URL } from "@/lib/paths";
import "./globals.css";

const pageUrl = new URL(assetPath("/"), SITE_URL);

export const metadata: Metadata = {
  metadataBase: new URL(SITE_URL),
  alternates: { canonical: pageUrl },
  title: "Sekiro Mod Manager · 重返苇名，由你执掌",
  description: "为《只狼：影逝二度》打造的开源模组工作台。暂存模组、查看覆盖、保存组合，以 NTFS 硬链接部署你的苇名。",
  icons: { icon: assetPath("/assets/favicon.svg") },
  openGraph: { title: "Sekiro Mod Manager", description: "你的模组，你的顺位，你的苇名。Windows 原生模组管理器，支持 CLI 与智能体 Skill。", type: "website", url: pageUrl },
};
export const viewport: Viewport = { themeColor: "#181a18" };
export default function RootLayout({ children }: { children: React.ReactNode }) {
  return <html lang="zh-CN"><body><I18nProvider>{children}</I18nProvider></body></html>;
}
