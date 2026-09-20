import React, { useState } from 'react';
import {
  FolderArchive,
  FolderOpen,
  Gamepad2,
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
  Play,
} from 'lucide-react';
import { SekiroLogo } from './SekiroLogo';
import { PresetSelector } from '../presets/PresetSelector';
import { openPathInExplorer, launchGame } from '../../api';
import type { AppSettings, ConflictReport, HealthReport, OverallHealth } from '../../types';

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
  const [isLaunchingGame, setIsLaunchingGame] = useState(false);

  const handleLaunchGame = async () => {
    if (!settings.game_dir) {
      if (showToast) {
        showToast('未配置游戏目录', '请先在右上方「设置」中指定《只狼》游戏安装目录', 'warning');
      } else {
        onOpenSettings();
      }
      return;
    }
    setIsLaunchingGame(true);
    try {
      await launchGame(settings.game_dir);
      if (showToast) {
        showToast('正在启动游戏', '已成功唤起只狼 sekiro.exe，祝受死愉快！', 'success');
      }
    } catch (err: any) {
      if (showToast) {
        showToast('启动游戏失败', String(err), 'error');
      }
    } finally {
      setIsLaunchingGame(false);
    }
  };

  const handleOpenDir = async (path: string, name: string) => {
    if (!path) {
      onOpenSettings();
      return;
    }
    try {
      await openPathInExplorer(path);
    } catch (err: any) {
      if (showToast) {
        showToast(`打开${name}失败`, String(err), 'error');
      }
    }
  };
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
    const iniFailItem = health?.items.find((i) =>
      i.name.toLowerCase().includes('modengine.ini') && i.status === 'Fail'
    );
    const hasModEngineConfig = health?.items.some((i) =>
      (i.category === 'ModEngine Config' || i.name.toLowerCase().includes('modengine')) && i.status === 'Pass'
    );
    const isEngineInstalled =
      dinputItem?.status === 'Pass' && !iniFailItem && hasModEngineConfig;

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
    <header className="h-16 border-b border-hairline-soft bg-canvas px-3.5 sm:px-4 flex items-center justify-between select-none z-30 flex-shrink-0 shadow-subtle gap-2">
      {/* Brand & Main Identity */}
      <div className="flex items-center gap-3 flex-shrink-0">
        <div className="flex items-center gap-2.5 flex-shrink-0">
          <SekiroLogo
            size={36}
            className="shadow-subtle rounded-full hover:scale-105 transition-transform duration-200 cursor-pointer flex-shrink-0"
            title="只狼 · 影逝二度 Mod Manager"
          />
          <div className="flex-shrink-0">
            <div className="flex items-center gap-1.5">
              <h1 className="font-extrabold text-sm tracking-tight text-ink-deep uppercase font-sans whitespace-nowrap">
                Sekiro Mod Manager
              </h1>
              <span className="text-[9px] px-1.5 py-0.5 rounded-full bg-surface-soft text-slate border border-hairline-soft font-mono font-bold tracking-wider whitespace-nowrap hidden 2xl:inline-block">
                NTFS PROJECTION
              </span>
            </div>
            <div className="text-[10px] text-steel font-sans tracking-tight hidden 2xl:flex items-center gap-1 whitespace-nowrap">
              <span>Direct Hardware Linker</span>
              <span className="text-stone">·</span>
              <span>Zero Disk Copy</span>
            </div>
          </div>
        </div>

        {/* Preset Selector Quick Trigger */}
        {settings.staging_dir && onPresetApplied && showToast && (
          <div className="flex items-center gap-2">
            <div className="h-5 w-px bg-hairline-soft hidden md:block" />
            <PresetSelector
              stagingDir={settings.staging_dir}
              onPresetApplied={onPresetApplied}
              showToast={showToast}
            />
          </div>
        )}

        {/* Quick Directory Shortcuts to Windows Explorer */}
        <div className="flex items-center gap-1 ml-0.5 flex-shrink-0">
          <button
            type="button"
            onClick={() => handleOpenDir(settings.game_dir, '游戏根目录')}
            className="flex items-center gap-1 px-2.5 py-1.5 rounded-full bg-surface-soft hover:bg-canvas border border-hairline-soft hover:border-hairline text-charcoal hover:text-ink text-xs font-semibold transition active:scale-[0.98] shadow-subtle group"
            title={settings.game_dir ? `在资源管理器中打开游戏根目录:\n${settings.game_dir}` : '点击配置游戏目录'}
          >
            <FolderOpen className="w-3.5 h-3.5 text-primary group-hover:scale-110 transition" />
            <span className="hidden xl:inline whitespace-nowrap">游戏目录</span>
          </button>

          <button
            type="button"
            onClick={() => handleOpenDir(settings.staging_dir, 'Mod 暂存库')}
            className="flex items-center gap-1 px-2.5 py-1.5 rounded-full bg-surface-soft hover:bg-canvas border border-hairline-soft hover:border-hairline text-charcoal hover:text-ink text-xs font-semibold transition active:scale-[0.98] shadow-subtle group"
            title={settings.staging_dir ? `在资源管理器中打开 Mod 暂存库:\n${settings.staging_dir}` : '点击配置暂存库'}
          >
            <FolderArchive className="w-3.5 h-3.5 text-attention group-hover:scale-110 transition" />
            <span className="hidden xl:inline whitespace-nowrap">Mod暂存库</span>
          </button>
        </div>
      </div>

      {/* Right Controls & Action CTA Cluster */}
      <div className="flex items-center gap-1.5 sm:gap-2 flex-shrink-0">
        {/* Health Doctor Trigger */}
        <button
          type="button"
          onClick={statusBadge.action}
          className={`flex items-center gap-1.5 px-2.5 py-1.5 rounded-full text-xs font-semibold border transition shadow-subtle ${statusBadge.wrapper}`}
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
          className={`flex items-center gap-1 px-2.5 py-1.5 rounded-full text-xs font-semibold border transition shadow-subtle ${
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

        {/* Divider */}
        <div className="h-5 w-px bg-hairline-soft mx-0.5 hidden sm:block" />

        {/* Core Actions */}
        {/* Secondary: Outlined Pill */}
        <button
          type="button"
          onClick={onRestore}
          disabled={isRestoring || isDeploying}
          className="border-2 border-ink-deep text-ink-deep hover:bg-black/5 px-3 py-1.5 rounded-full font-bold text-xs transition flex items-center gap-1.5 disabled:opacity-40 disabled:cursor-not-allowed active:scale-[0.98] whitespace-nowrap shadow-subtle"
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
          className="bg-ink-button text-white hover:bg-charcoal px-3.5 py-2 rounded-full font-bold text-xs shadow-sm transition flex items-center gap-1.5 active:scale-[0.98] disabled:opacity-40 disabled:cursor-not-allowed whitespace-nowrap"
          title="一键秒级 NTFS 硬链接投影部署"
        >
          {isDeploying ? (
            <Loader2 className="w-3.5 h-3.5 animate-spin text-white" />
          ) : (
            <Zap className="w-3.5 h-3.5 text-warning fill-warning" />
          )}
          <span>一键部署</span>
        </button>

        {/* Launch Game CTA Button */}
        <button
          type="button"
          onClick={handleLaunchGame}
          disabled={isLaunchingGame || !settings.game_dir}
          className="bg-emerald-700 hover:bg-emerald-800 text-white px-3 py-2 rounded-full font-bold text-xs shadow-sm transition flex items-center gap-1.5 active:scale-[0.98] disabled:opacity-40 disabled:cursor-not-allowed whitespace-nowrap"
          title={settings.game_dir ? `启动只狼 (${settings.game_dir}\\sekiro.exe)` : '请先在设置中指定游戏目录'}
        >
          {isLaunchingGame ? (
            <Loader2 className="w-3.5 h-3.5 animate-spin text-white" />
          ) : (
            <Play className="w-3.5 h-3.5 text-white fill-white" />
          )}
          <span>启动游戏</span>
        </button>

        {/* Settings Icon */}
        <button
          type="button"
          onClick={onOpenSettings}
          className="w-8 h-8 rounded-full border border-hairline-soft hover:bg-surface-soft text-charcoal hover:text-ink-deep flex items-center justify-center transition ml-0.5 flex-shrink-0 shadow-subtle"
          title="首选项与目录设置"
        >
          <SettingsIcon className="w-3.5 h-3.5" />
        </button>
      </div>
    </header>
  );
};
