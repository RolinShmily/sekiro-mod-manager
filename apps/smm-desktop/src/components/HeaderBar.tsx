import React from 'react';
import {
  FolderArchive,
  HardDrive,
  RotateCcw,
  Zap,
  AlertTriangle,
  Settings as SettingsIcon,
  ShieldCheck,
  Activity,
  ShieldAlert,
  Loader2,
  PackageOpen,
  Boxes,
} from 'lucide-react';
import { SekiroLogo } from './SekiroLogo';
import { PresetSelector } from './PresetSelector';
import type { AppSettings, ConflictReport, HealthReport, OverallHealth } from '../types';

interface HeaderBarProps {
  settings: AppSettings;
  health: HealthReport | null;
  conflicts: ConflictReport | null;
  isDeploying: boolean;
  isRestoring: boolean;
  onOpenSettings: () => void;
  onOpenDoctor: () => void;
  onOpenConflicts: () => void;
  onOpenImportModpack?: () => void;
  onOpenExportModpack?: () => void;
  onPresetApplied?: () => void;
  showToast?: (title: string, message: string, type: 'success' | 'error' | 'warning' | 'info') => void;
  onDeploy: () => void;
  onRestore: () => void;
}

export const HeaderBar: React.FC<HeaderBarProps> = ({
  settings,
  health,
  conflicts,
  isDeploying,
  isRestoring,
  onOpenSettings,
  onOpenDoctor,
  onOpenConflicts,
  onOpenImportModpack,
  onOpenExportModpack,
  onPresetApplied,
  showToast,
  onDeploy,
  onRestore,
}) => {
  const getStatusBadge = () => {
    // 1. 游戏路径未绑定或目录不存在时，展示温和稳健的「待配置游戏」（Amber 琥珀色调），杜绝未配置时直接报红惊扰用户
    const gameDirItem = health?.items.find((i) =>
      i.name.toLowerCase().includes('game directory')
    );
    const isGameBound = Boolean(
      settings.game_dir &&
      settings.game_dir.trim() !== '' &&
      gameDirItem?.status !== 'Fail'
    );

    if (!isGameBound) {
      return {
        label: '待配置游戏',
        wrapper: 'bg-amber-50 text-amber-900 border-amber-200 hover:bg-amber-100/80',
        dot: 'bg-amber-500 shadow-[0_0_6px_rgba(245,158,11,0.6)]',
        icon: <Activity className="w-3.5 h-3.5 text-amber-600" />,
        action: onOpenSettings,
        title: '尚未配置有效的《只狼》游戏目录，点击快速指定',
      };
    }

    const dinputItem = health?.items.find((i) =>
      i.name.toLowerCase().includes('dinput8')
    );
    const iniItem = health?.items.find((i) =>
      i.name.toLowerCase().includes('modengine.ini')
    );
    const isEngineInstalled =
      dinputItem?.status === 'Pass' && iniItem?.status === 'Pass';

    // 2. 已装配 ModEngine 且环境健康
    if (isEngineInstalled && health?.overall_status === 'Healthy') {
      return {
        label: '环境健康',
        wrapper: 'bg-emerald-50 text-emerald-800 border-emerald-200 hover:bg-emerald-100/80',
        dot: 'bg-[#31a24c] shadow-[0_0_6px_rgba(49,162,76,0.6)]',
        icon: <ShieldCheck className="w-3.5 h-3.5 text-[#31a24c]" />,
        action: onOpenDoctor,
        title: 'ModEngine 运行环境完整健康，点击查看详情',
      };
    }

    // 3. 游戏路径已绑定但未装配 ModEngine
    if (!isEngineInstalled) {
      return {
        label: '待装配引擎',
        wrapper: 'bg-amber-50 text-amber-900 border-amber-200 hover:bg-amber-100/80',
        dot: 'bg-amber-500 shadow-[0_0_6px_rgba(245,158,11,0.6)]',
        icon: <ShieldAlert className="w-3.5 h-3.5 text-amber-600" />,
        action: onOpenDoctor,
        title: '检测到未装配 ModEngine 核心补丁，点击一键自动注入装配',
      };
    }

    // 4. 其余降级提示（如跨分区硬链接提示）
    if (health?.overall_status === 'Degraded') {
      return {
        label: '配置提示',
        wrapper: 'bg-amber-50 text-amber-900 border-amber-200 hover:bg-amber-100/80',
        dot: 'bg-amber-500 shadow-[0_0_6px_rgba(245,158,11,0.6)]',
        icon: <Activity className="w-3.5 h-3.5 text-amber-600" />,
        action: onOpenDoctor,
        title: '环境存在优化建议，点击查看诊断详情',
      };
    }

    // 5. 异常需修复
    return {
      label: '需修复',
      wrapper: 'bg-rose-50 text-rose-800 border-rose-200 hover:bg-rose-100/80',
      dot: 'bg-[#e41e3f] animate-pulse shadow-[0_0_6px_rgba(228,30,63,0.6)]',
      icon: <ShieldAlert className="w-3.5 h-3.5 text-[#e41e3f]" />,
      action: onOpenDoctor,
      title: '环境异常，点击展开 ModEngine 环境诊断与修复',
    };
  };

  const statusBadge = getStatusBadge();
  const conflictCount = conflicts?.total_conflicts || 0;

  function truncatePath(path: string, maxLen = 28): string {
    if (!path) return '未设定';
    if (path.length <= maxLen) return path;
    return '...' + path.slice(-(maxLen - 3));
  }

  return (
    <header className="h-16 border-b border-hairline-soft bg-canvas px-6 flex items-center justify-between select-none z-30 flex-shrink-0 shadow-subtle gap-4">
      {/* Brand & Main Identity */}
      <div className="flex items-center gap-4 flex-shrink-0">
        <div className="flex items-center gap-3 flex-shrink-0">
          <SekiroLogo
            size={38}
            className="shadow-subtle rounded-full hover:scale-105 transition-transform duration-200 cursor-pointer flex-shrink-0"
            title="只狼 · 影逝二度 Mod Manager"
          />
          <div className="flex-shrink-0">
            <div className="flex items-center gap-2">
              <h1 className="font-extrabold text-sm tracking-tight text-ink-deep uppercase font-sans whitespace-nowrap">
                Sekiro Mod Manager
              </h1>
              <span className="text-[10px] px-2 py-0.5 rounded-full bg-surface-soft text-slate border border-hairline-soft font-mono font-bold tracking-wider whitespace-nowrap hidden sm:inline-block">
                NTFS PROJECTION
              </span>
            </div>
            <div className="text-[11px] text-steel font-sans tracking-tight flex items-center gap-1.5 whitespace-nowrap">
              <span>Direct Hardware Linker</span>
              <span className="text-stone">·</span>
              <span>Zero Disk Copy</span>
            </div>
          </div>
        </div>

        {/* Preset Selector Quick Trigger */}
        {settings.staging_dir && onPresetApplied && showToast && (
          <div className="flex items-center gap-3">
            <div className="h-6 w-px bg-hairline-soft hidden lg:block" />
            <PresetSelector
              stagingDir={settings.staging_dir}
              onPresetApplied={onPresetApplied}
              showToast={showToast}
            />
          </div>
        )}

        {/* Path Quick Indicators */}
        <div className="hidden 2xl:flex items-center gap-2 ml-2 flex-shrink-0">
          {/* Game Dir Pill */}
          <button
            type="button"
            onClick={onOpenSettings}
            className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-surface-soft hover:bg-[#e6ebf0] border border-hairline-soft text-charcoal hover:text-ink transition text-xs group"
            title="点击配置《只狼》游戏根目录"
          >
            <HardDrive className="w-3.5 h-3.5 text-steel group-hover:text-ink transition" />
            <span className="text-[11px] font-semibold text-slate">游戏:</span>
            <span className="text-ink font-mono text-[11px] font-medium">
              {truncatePath(settings.game_dir, 22)}
            </span>
          </button>

          {/* Staging Dir Pill */}
          <button
            type="button"
            onClick={onOpenSettings}
            className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-surface-soft hover:bg-[#e6ebf0] border border-hairline-soft text-charcoal hover:text-ink transition text-xs group"
            title="点击配置模组暂存仓库 (Staging)"
          >
            <FolderArchive className="w-3.5 h-3.5 text-steel group-hover:text-ink transition" />
            <span className="text-[11px] font-semibold text-slate">暂存库:</span>
            <span className="text-ink font-mono text-[11px] font-medium">
              {truncatePath(settings.staging_dir, 20)}
            </span>
          </button>
        </div>
      </div>

      {/* Right Controls & Action CTA Cluster */}
      <div className="flex items-center gap-2.5 flex-shrink-0">
        {/* Health Doctor Trigger */}
        <button
          type="button"
          onClick={statusBadge.action}
          className={`flex items-center gap-2 px-3.5 py-1.5 rounded-full text-xs font-semibold border transition shadow-subtle ${statusBadge.wrapper}`}
          title={statusBadge.title}
        >
          <span className={`w-2 h-2 rounded-full flex-shrink-0 ${statusBadge.dot}`} />
          {statusBadge.icon}
          <span className="whitespace-nowrap">{statusBadge.label}</span>
        </button>

        {/* Conflicts Badge Trigger */}
        <button
          type="button"
          onClick={onOpenConflicts}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-full text-xs font-semibold border transition shadow-subtle ${
            conflictCount > 0
              ? 'bg-amber-50 text-amber-900 border-amber-300 hover:bg-amber-100'
              : 'bg-surface-soft text-steel border-hairline-soft hover:bg-white hover:text-ink'
          }`}
          title="查看模组文件碰撞与遮蔽矩阵"
        >
          <AlertTriangle
            className={`w-3.5 h-3.5 flex-shrink-0 ${conflictCount > 0 ? 'text-attention' : 'text-stone'}`}
          />
          <span className="whitespace-nowrap">冲突</span>
          <span
            className={`px-1.5 py-0.2 rounded-full text-[10px] font-bold ${
              conflictCount > 0 ? 'bg-attention text-white' : 'bg-hairline-soft text-slate'
            }`}
          >
            {conflictCount}
          </span>
        </button>

        {/* Modpack Operation Buttons */}
        <div className="flex items-center gap-1.5">
          <button
            type="button"
            onClick={onOpenImportModpack}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-surface-soft text-charcoal border border-hairline-soft hover:bg-[#e6ebf0] hover:text-ink text-xs font-semibold transition active:scale-[0.98] shadow-subtle"
            title="导入 .smmpack 模组整合包"
          >
            <PackageOpen className="w-3.5 h-3.5 text-primary flex-shrink-0" />
            <span className="hidden xl:inline whitespace-nowrap">导入整合包</span>
            <span className="xl:hidden whitespace-nowrap">导入包</span>
          </button>
          <button
            type="button"
            onClick={onOpenExportModpack}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-full bg-surface-soft text-charcoal border border-hairline-soft hover:bg-[#e6ebf0] hover:text-ink text-xs font-semibold transition active:scale-[0.98] shadow-subtle"
            title="打包导出当前选定模组为 .smmpack 整合包"
          >
            <Boxes className="w-3.5 h-3.5 text-primary flex-shrink-0" />
            <span className="hidden xl:inline whitespace-nowrap">导出整合包</span>
            <span className="xl:hidden whitespace-nowrap">导出包</span>
          </button>
        </div>

        {/* Divider */}
        <div className="h-6 w-px bg-hairline-soft mx-0.5 hidden sm:block" />

        {/* Core Dual-CTA Pattern (DESIGN.md signature) */}
        {/* Secondary: Outlined Pill */}
        <button
          type="button"
          onClick={onRestore}
          disabled={isRestoring || isDeploying}
          className="border-2 border-ink-deep text-ink-deep hover:bg-black/5 px-4 sm:px-5 py-2 rounded-full font-bold text-xs transition flex items-center gap-1.5 disabled:opacity-40 disabled:cursor-not-allowed active:scale-[0.98] whitespace-nowrap shadow-subtle"
          title="清空游戏 mods 目录中的硬链接与部署文件"
        >
          {isRestoring ? (
            <Loader2 className="w-3.5 h-3.5 animate-spin text-ink-deep" />
          ) : (
            <RotateCcw className="w-3.5 h-3.5 text-ink-deep" />
          )}
          <span>还原纯净</span>
        </button>

        {/* Primary: Black Pill */}
        <button
          type="button"
          onClick={onDeploy}
          disabled={isDeploying || isRestoring}
          className="bg-ink-button text-white hover:bg-charcoal px-5 sm:px-6 py-2.5 rounded-full font-bold text-xs shadow-sm transition flex items-center gap-2 active:scale-[0.98] disabled:opacity-40 disabled:cursor-not-allowed whitespace-nowrap"
          title="一键秒级 NTFS 硬链接投影部署"
        >
          {isDeploying ? (
            <Loader2 className="w-3.5 h-3.5 animate-spin text-white" />
          ) : (
            <Zap className="w-3.5 h-3.5 text-warning fill-warning" />
          )}
          <span>一键秒级部署</span>
        </button>

        {/* Settings Icon */}
        <button
          type="button"
          onClick={onOpenSettings}
          className="w-9 h-9 rounded-full border border-hairline-soft hover:bg-surface-soft text-charcoal hover:text-ink-deep flex items-center justify-center transition ml-0.5 flex-shrink-0 shadow-subtle"
          title="首选项与目录设置"
        >
          <SettingsIcon className="w-4 h-4" />
        </button>
      </div>
    </header>
  );
};
