"use client";

import { useState } from "react";
import * as Tabs from "@radix-ui/react-tabs";
import { ArrowRight, Folder, ShieldCheck } from "lucide-react";
import { useI18n } from "./i18n";

const steps = [
  { title: "stepImport", subtitle: "stepImportSub", source: "katana.zip", target: "staging / katana", operation: "NORMALIZE", command: "smm import katana.zip --dry-run", zh: { source: "下载的模组包", target: "独立暂存目录", comment: "# 查看识别结果，不安装模组", explanation: "导入前可以用 CLI 预览识别结果。暂存目录用于管理模组，导入本身不会部署到游戏。" }, en: { source: "Downloaded mod archive", target: "Independent staging", comment: "# Inspect without installing the mod", explanation: "Preview asset recognition with the CLI before importing. Staging manages mod files; importing does not deploy to the game." } },
  { title: "stepOrder", subtitle: "stepOrderSub", source: "priority 10 / 20", target: "winning assets", operation: "REVIEW", command: "smm --json conflicts", zh: { source: "数值越小越优先", target: "确认胜出文件", comment: "# 分析覆盖，不修改游戏", explanation: "检查重合路径、胜出模组与冲突级别。按意图调整顺位或禁用资产；参数表需要你选择，不会自动合并。" }, en: { source: "Lower numbers win", target: "Confirm the winning files", comment: "# Inspect overrides without changing the game", explanation: "Check overlapping paths, winning mods, and conflict levels. Set priorities or disable assets. Parameter tables need your decision; they are not merged." } },
  { title: "stepDeploy", subtitle: "stepDeploySub", source: "staging / katana", target: "Sekiro / mods", operation: "HARD LINK", command: "smm --json plan", zh: { source: "已确认的模组资产", target: "游戏加载目录", comment: "# 先看计划，确认后再部署", explanation: "同一 NTFS 卷优先建立硬链接，跨卷回退为复制。确认计划后，使用 APP 的部署按钮或 CLI deploy 执行。" }, en: { source: "Reviewed mod assets", target: "Game loading directory", comment: "# Review the plan before deploying", explanation: "SMM uses hard links on the same NTFS volume and copies across volumes. Review the plan, then deploy through the app or CLI deploy." } },
] as const;

export function Workflow() {
  const { locale, t } = useI18n();
  const [active, setActive] = useState("0");
  return <section className="workflow section-pad" id="workflow" aria-labelledby="workflow-title"><div className="container">
    <div className="section-heading reveal"><p className="eyebrow">03 / THE WAY TO DEPLOY</p><div className="heading-row"><h2 id="workflow-title">{t("workflowTitle")}</h2><p>{t("workflowDescription")}</p></div></div>
    <Tabs.Root className="workflow-layout reveal" value={active} onValueChange={setActive}>
      <Tabs.List className="workflow-steps" aria-label={t("stepsLabel")}>{steps.map((step, index) => <Tabs.Trigger key={index} id={`step-tab-${index}`} className="workflow-step" value={String(index)}><span className="step-number">0{index + 1}</span><span><strong>{t(step.title)}</strong><span>{t(step.subtitle)}</span></span><ArrowRight className="icon" /></Tabs.Trigger>)}</Tabs.List>
      {steps.map((step, index) => <Tabs.Content key={index} className="workflow-panel" value={String(index)}><div className="panel-top"><span>{t("workflowPreview")}</span><span>STAGING → GAME</span></div><div className="file-map"><div className="file-node"><Folder className="icon" /><strong>{step.source}</strong><span>{step[locale].source}</span></div><div className="file-connection"><ArrowRight className="icon" /><span>{step.operation}</span></div><div className="file-node"><Folder className="icon" /><strong>{step.target}</strong><span>{step[locale].target}</span></div></div><div className="flow-code"><span className="code-comment">{step[locale].comment}</span><code>{step.command}</code></div><p id="flow-explanation">{step[locale].explanation}</p><p className="workflow-disclaimer">{t("workflowDisclaimer")}</p></Tabs.Content>)}
    </Tabs.Root>
    <p className="workflow-note"><ShieldCheck className="icon" /><span>{t("workflowNote")}</span></p>
  </div></section>;
}
