"use client";

import { useState, useRef } from "react";
import * as Tabs from "@radix-ui/react-tabs";
import {
  ArrowRight,
  ShieldCheck,
  FolderInput,
  FolderTree,
  FileArchive,
  SlidersHorizontal,
  HardDrive,
  Link2,
  Copy,
  Check,
  Terminal,
  ArrowRightLeft,
  Info,
} from "lucide-react";
import { useI18n } from "./i18n";

const steps = [
  {
    title: "stepImport",
    subtitle: "stepImportSub",
    source: "katana.zip",
    target: "staging / katana",
    operation: "NORMALIZE",
    command: "smm import katana.zip --dry-run",
    SourceIcon: FileArchive,
    TargetIcon: FolderTree,
    ConnectIcon: ArrowRight,
    StepIcon: FolderInput,
    zh: {
      source: "下载的模组包",
      target: "独立暂存目录",
      comment: "# 查看识别结果，不安装模组",
      explanation:
        "导入前可以用 CLI 预览识别结果。暂存目录用于管理模组，导入本身不会部署到游戏。",
    },
    en: {
      source: "Downloaded mod archive",
      target: "Independent staging",
      comment: "# Inspect without installing the mod",
      explanation:
        "Preview asset recognition with the CLI before importing. Staging manages mod files; importing does not deploy to the game.",
    },
  },
  {
    title: "stepOrder",
    subtitle: "stepOrderSub",
    source: "priority 10 / 20",
    target: "winning assets",
    operation: "REVIEW",
    command: "smm --json conflicts",
    SourceIcon: SlidersHorizontal,
    TargetIcon: ShieldCheck,
    ConnectIcon: ArrowRight,
    StepIcon: SlidersHorizontal,
    zh: {
      source: "数值越小越优先",
      target: "确认胜出文件",
      comment: "# 分析覆盖，不修改游戏",
      explanation:
        "检查重合路径、胜出模组与冲突级别。按意图调整顺位或禁用资产；参数表需要你选择，不会自动合并。",
    },
    en: {
      source: "Lower numbers win",
      target: "Confirm the winning files",
      comment: "# Inspect overrides without changing the game",
      explanation:
        "Check overlapping paths, winning mods, and conflict levels. Set priorities or disable assets. Parameter tables need your decision; they are not merged.",
    },
  },
  {
    title: "stepDeploy",
    subtitle: "stepDeploySub",
    source: "staging / katana",
    target: "Sekiro / mods",
    operation: "HARD LINK",
    command: "smm --json plan",
    SourceIcon: FolderTree,
    TargetIcon: HardDrive,
    ConnectIcon: Link2,
    StepIcon: HardDrive,
    zh: {
      source: "已确认的模组资产",
      target: "游戏加载目录",
      comment: "# 先看计划，确认后再部署",
      explanation:
        "同一 NTFS 卷优先建立硬链接，跨卷回退为复制。确认计划后，使用 APP 的部署按钮或 CLI deploy 执行。",
    },
    en: {
      source: "Reviewed mod assets",
      target: "Game loading directory",
      comment: "# Review the plan before deploying",
      explanation:
        "SMM uses hard links on the same NTFS volume and copies across volumes. Review the plan, then deploy through the app or CLI deploy.",
    },
  },
] as const;

export function Workflow() {
  const { locale, t } = useI18n();
  const [active, setActive] = useState("0");
  const [copiedIndex, setCopiedIndex] = useState<number | null>(null);
  const copyTimer = useRef<ReturnType<typeof setTimeout> | null>(null);

  async function copyCommand(cmd: string, index: number) {
    try {
      await navigator.clipboard.writeText(cmd);
      setCopiedIndex(index);
      if (copyTimer.current) clearTimeout(copyTimer.current);
      copyTimer.current = setTimeout(() => setCopiedIndex(null), 3000);
    } catch (e) {
      console.warn("Failed to copy command", e);
    }
  }

  return (
    <section className="workflow section-pad" id="workflow" aria-labelledby="workflow-title">
      <div className="container">
        <div className="section-heading reveal">
          <p className="eyebrow">03 / THE WAY TO DEPLOY</p>
          <div className="heading-row">
            <h2 id="workflow-title">{t("workflowTitle")}</h2>
            <p>{t("workflowDescription")}</p>
          </div>
        </div>
        <Tabs.Root className="workflow-layout reveal" value={active} onValueChange={setActive}>
          <Tabs.List className="workflow-steps" aria-label={t("stepsLabel")}>
            {steps.map((step, index) => {
              const Icon = step.StepIcon;
              return (
                <Tabs.Trigger
                  key={index}
                  id={`step-tab-${index}`}
                  className="workflow-step"
                  value={String(index)}
                >
                  <span className="step-number">0{index + 1}</span>
                  <span>
                    <strong className="flex items-center gap-2">
                      <Icon size={16} className="text-accent shrink-0" />
                      {t(step.title)}
                    </strong>
                    <span>{t(step.subtitle)}</span>
                  </span>
                  <ArrowRight className="icon" />
                </Tabs.Trigger>
              );
            })}
          </Tabs.List>
          {steps.map((step, index) => {
            const SourceIcon = step.SourceIcon;
            const TargetIcon = step.TargetIcon;
            const ConnectIcon = step.ConnectIcon;
            const isCopied = copiedIndex === index;
            return (
              <Tabs.Content key={index} className="workflow-panel" value={String(index)}>
                <div className="panel-top">
                  <span className="flex items-center gap-1.5">
                    <Terminal size={14} className="text-accent" />
                    {t("workflowPreview")}
                  </span>
                  <span className="flex items-center gap-1.5">
                    <ArrowRightLeft size={13} />
                    STAGING → GAME
                  </span>
                </div>
                <div className="file-map">
                  <div className="file-node">
                    <SourceIcon className="icon mb-1.5 text-accent" />
                    <strong>{step.source}</strong>
                    <span>{step[locale].source}</span>
                  </div>
                  <div className="file-connection">
                    <ConnectIcon className="icon" />
                    <span>{step.operation}</span>
                  </div>
                  <div className="file-node">
                    <TargetIcon className="icon mb-1.5 text-[#3b5e43]" />
                    <strong>{step.target}</strong>
                    <span>{step[locale].target}</span>
                  </div>
                </div>
                <div className="flow-code group relative">
                  <span className="code-comment">{step[locale].comment}</span>
                  <div className="flex items-center justify-between gap-2 mt-1">
                    <code>{step.command}</code>
                    <button
                      className="flow-code-copy"
                      onClick={() => copyCommand(step.command, index)}
                      aria-label="Copy CLI command"
                      title="Copy CLI command"
                    >
                      {isCopied ? <Check size={14} /> : <Copy size={14} />}
                    </button>
                  </div>
                </div>
                <p id="flow-explanation">{step[locale].explanation}</p>
                <p className="workflow-disclaimer flex items-center gap-1.5">
                  <Info size={13} className="shrink-0 text-ink-muted" />
                  <span>{t("workflowDisclaimer")}</span>
                </p>
              </Tabs.Content>
            );
          })}
        </Tabs.Root>
        <p className="workflow-note">
          <ShieldCheck className="icon" />
          <span>{t("workflowNote")}</span>
        </p>
      </div>
    </section>
  );
}
