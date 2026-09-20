import React, { useMemo } from 'react';
import {
  AlertTriangle,
  FileCode,
  FolderTree,
  Lock,
  Settings as SettingsIcon,
  Sparkles,
} from 'lucide-react';
import type { AssetEntry } from '../../types';
import { formatBytes } from '../../utils/format';

interface AssetGroup {
  folder: string;
  count: number;
  totalSize: number;
  entries: AssetEntry[];
}

interface ModAssetsTabProps {
  assets: AssetEntry[];
}

/** 归一化资产层级树 (TAB 1): 按顶层目录分组展示模组投影文件。 */
export const ModAssetsTab: React.FC<ModAssetsTabProps> = ({ assets }) => {
  const assetGroups = useMemo<AssetGroup[]>(() => {
    const groups: Record<string, AssetGroup> = {};

    for (const asset of assets) {
      const parts = asset.relative_path.split('/');
      const folder = parts.length > 1 ? parts[0] : 'root';

      if (!groups[folder]) {
        groups[folder] = { folder, count: 0, totalSize: 0, entries: [] };
      }
      groups[folder].count += 1;
      groups[folder].totalSize += asset.file_size;
      groups[folder].entries.push(asset);
    }

    return Object.values(groups).sort((a, b) => b.count - a.count);
  }, [assets]);

  return (
    <div className="space-y-4">
      <div className="text-xs text-steel flex items-center justify-between font-sans">
        <span className="font-semibold text-charcoal flex items-center gap-1.5">
          <FolderTree className="w-4 h-4 text-steel" />
          <span>只狼原生资产目录标准投影</span>
        </span>
        <div className="flex items-center gap-2 text-[11px]">
          <span className="flex items-center gap-1 px-2.5 py-0.5 rounded-full bg-rose-50 border border-rose-200">
            <AlertTriangle className="w-3 h-3 text-critical" />
            <span className="font-bold text-critical">[CRITICAL] 核心参数</span>
          </span>
          <span className="flex items-center gap-1 px-2.5 py-0.5 rounded-full bg-amber-50 border border-amber-200">
            <Lock className="w-3 h-3 text-attention" />
            <span className="font-bold text-attention">[EXCLUSIVE SLOT] 独占槽位</span>
          </span>
        </div>
      </div>

      {assetGroups.map((group) => (
        <div
          key={group.folder}
          className="rounded-xxl border border-hairline-soft bg-canvas overflow-hidden shadow-card"
        >
          <div className="px-5 py-3 bg-surface-soft border-b border-hairline-soft flex items-center justify-between text-xs font-mono">
            <div className="flex items-center gap-2">
              <FolderTree className="w-4 h-4 text-steel flex-shrink-0" />
              <span className="text-ink-deep font-bold tracking-wide">{group.folder}/</span>
              <span className="text-stone text-[11px]">({group.count} 文件)</span>
            </div>
            <span className="text-steel text-[11px]">{formatBytes(group.totalSize)}</span>
          </div>

          <div className="divide-y divide-hairline-soft/60">
            {group.entries.map((asset) => {
              const isConfig =
                asset.relative_path.endsWith('.ini') ||
                asset.relative_path.endsWith('.json') ||
                asset.relative_path.endsWith('.toml') ||
                asset.relative_path.endsWith('.xml') ||
                asset.relative_path.includes('config');

              return (
                <div
                  key={asset.relative_path}
                  className="px-5 py-2.5 flex items-center justify-between hover:bg-surface-soft/50 transition text-xs font-mono"
                >
                  <div className="flex items-center gap-2.5 min-w-0">
                    {isConfig ? (
                      <SettingsIcon className="w-3.5 h-3.5 text-steel flex-shrink-0" />
                    ) : (
                      <FileCode className="w-3.5 h-3.5 text-steel flex-shrink-0" />
                    )}
                    <span className="text-ink-deep truncate font-medium">
                      {asset.relative_path}
                    </span>

                    {asset.is_critical && (
                      <span
                        className="px-2.5 py-0.5 rounded-full text-[10px] font-bold bg-rose-50 text-critical border border-rose-200 animate-pulse flex-shrink-0 flex items-center gap-1"
                        title="核心引擎参数文件 (gameparam.parambnd.dcx) - 冲突时后覆盖者决定一切数值"
                      >
                        <AlertTriangle className="w-2.5 h-2.5 text-critical" />
                        <span>CRITICAL PARAM</span>
                      </span>
                    )}

                    {asset.is_exclusive_slot && (
                      <span
                        className="px-2.5 py-0.5 rounded-full text-[10px] font-bold bg-amber-50 text-amber-900 border border-amber-300 flex-shrink-0 flex items-center gap-1"
                        title="独占武器或角色模型槽位 (例如 wp_a_0300 楔丸) - 冲突时仅胜出者模型可见"
                      >
                        <Lock className="w-2.5 h-2.5 text-amber-700" />
                        <Sparkles className="w-2.5 h-2.5 text-amber-500" />
                        <span>EXCLUSIVE SLOT</span>
                      </span>
                    )}
                  </div>

                  <div className="text-stone text-[11px] flex-shrink-0 ml-4">
                    {formatBytes(asset.file_size)}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      ))}
    </div>
  );
};