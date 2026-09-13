import React, { useState, useEffect } from 'react';
import {
  X,
  Settings,
  HardDrive,
  FolderArchive,
  FolderOpen,
  Save,
  ExternalLink,
} from 'lucide-react';
import { SekiroLogo } from './SekiroLogo';
import { pickFolder, openPathInExplorer } from '../api';
import type { AppSettings } from '../types';

interface SettingsModalProps {
  isOpen: boolean;
  settings: AppSettings;
  onClose: () => void;
  onSave: (newSettings: AppSettings) => void;
}

export const SettingsModal: React.FC<SettingsModalProps> = ({
  isOpen,
  settings,
  onClose,
  onSave,
}) => {
  const [gameDir, setGameDir] = useState(settings.game_dir);
  const [stagingDir, setStagingDir] = useState(settings.staging_dir);

  useEffect(() => {
    setGameDir(settings.game_dir);
    setStagingDir(settings.staging_dir);
  }, [settings]);

  if (!isOpen) return null;

  function handleSave() {
    onSave({
      game_dir: gameDir.trim(),
      staging_dir: stagingDir.trim(),
    });
  }

  function useSteamPreset() {
    setGameDir('C:\\Program Files (x86)\\Steam\\steamapps\\common\\Sekiro');
  }

  async function handleBrowseGameDir() {
    const selected = await pickFolder('选择《只狼》游戏安装主目录 (含 sekiro.exe)', gameDir);
    if (selected) {
      setGameDir(selected);
    }
  }

  async function handleBrowseStagingDir() {
    const selected = await pickFolder('选择模组暂存仓库目录 (Staging)', stagingDir);
    if (selected) {
      setStagingDir(selected);
    }
  }

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4">
      <div className="w-[640px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={34} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                首选项与路径配置
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO MOD MANAGER PREFERENCES</span>
            </div>
          </div>
          <button
            type="button"
            onClick={onClose}
            className="w-8 h-8 rounded-full flex items-center justify-center text-steel hover:text-ink hover:bg-surface-soft transition"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Modal Body */}
        <div className="p-6 space-y-5">
          {/* Game Directory Field */}
          <div className="space-y-1.5">
            <div className="flex items-center justify-between">
              <label className="text-xs font-bold text-ink-deep flex items-center gap-1.5 font-sans">
                <HardDrive className="w-3.5 h-3.5 text-steel" />
                <span>《只狼》游戏安装主目录 (含 sekiro.exe):</span>
              </label>
              <button
                type="button"
                onClick={useSteamPreset}
                className="text-[11px] font-semibold text-primary hover:underline"
              >
                填充 Steam 默认路径
              </button>
            </div>
            <div className="flex gap-2">
              <input
                type="text"
                value={gameDir}
                onChange={(e) => setGameDir(e.target.value)}
                placeholder="例如: C:\Program Files (x86)\Steam\steamapps\common\Sekiro"
                className="flex-1 px-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
              />
              <button
                type="button"
                onClick={handleBrowseGameDir}
                className="px-4 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle"
                title="打开 Windows 资源管理器选择文件夹"
              >
                <FolderOpen className="w-3.5 h-3.5 text-steel" />
                <span>浏览...</span>
              </button>
              <button
                type="button"
                onClick={() => gameDir && openPathInExplorer(gameDir)}
                disabled={!gameDir}
                className="px-3.5 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle disabled:opacity-40"
                title="在 Windows 资源管理器中打开此游戏目录"
              >
                <ExternalLink className="w-3.5 h-3.5 text-steel" />
                <span>打开</span>
              </button>
            </div>
            <div className="text-[11px] text-steel font-sans">
              ModEngine 将以此目录下的 <code className="text-ink font-mono bg-surface-soft px-1.5 py-0.5 rounded border border-hairline-soft">mods/</code> 作为目标进行 NTFS 硬链接投影。
            </div>
          </div>

          {/* Staging Directory Field */}
          <div className="space-y-1.5">
            <div className="flex items-center justify-between">
              <label className="text-xs font-bold text-ink-deep flex items-center gap-1.5 font-sans">
                <FolderArchive className="w-3.5 h-3.5 text-steel" />
                <span>模组暂存仓库目录 (Staging):</span>
              </label>
            </div>
            <div className="flex gap-2">
              <input
                type="text"
                value={stagingDir}
                onChange={(e) => setStagingDir(e.target.value)}
                placeholder="例如: D:\SekiroModsStaging 或 D:\SteamLibrary\steamapps\common\Sekiro\mods_staging"
                className="flex-1 px-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
              />
              <button
                type="button"
                onClick={handleBrowseStagingDir}
                className="px-4 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle"
                title="打开 Windows 资源管理器选择文件夹"
              >
                <FolderOpen className="w-3.5 h-3.5 text-steel" />
                <span>浏览...</span>
              </button>
              <button
                type="button"
                onClick={() => stagingDir && openPathInExplorer(stagingDir)}
                disabled={!stagingDir}
                className="px-3.5 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle disabled:opacity-40"
                title="在 Windows 资源管理器中打开此暂存库目录"
              >
                <ExternalLink className="w-3.5 h-3.5 text-steel" />
                <span>打开</span>
              </button>
            </div>
            <div className="text-[11px] text-steel font-sans">
              存放已归一化的 Mod 仓库。建议与游戏目录置于同一磁盘分区以激活 NTFS 零开销硬链接。
            </div>
          </div>
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-4 border-t border-hairline-soft bg-surface-soft flex items-center justify-end gap-3">
          <button
            type="button"
            onClick={onClose}
            className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-canvas text-xs font-bold transition shadow-subtle"
          >
            取消
          </button>
          <button
            type="button"
            onClick={handleSave}
            className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98]"
          >
            <Save className="w-4 h-4" />
            <span>保存配置并重新扫描</span>
          </button>
        </div>
      </div>
    </div>
  );
};
