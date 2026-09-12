import React, { useState, useMemo } from 'react';
import {
  Search,
  ArrowUp,
  ArrowDown,
  FolderInput,
  FolderArchive,
  AlertTriangle,
  Trash2,
  X,
  PackageOpen,
  Boxes,
  Zap,
  DownloadCloud,
  Loader2,
} from 'lucide-react';
import type { ModSummary } from '../types';
import { formatBytes, getCategoryBadgeClass, getCategoryLabel } from '../utils/format';

interface ModListProps {
  mods: ModSummary[];
  selectedModId: string | null;
  conflictModIds: Set<string>;
  isLoading: boolean;
  isProvisioningEngine?: boolean;
  onSelect: (modId: string) => void;
  onToggle: (modId: string, enabled: boolean) => void;
  onMoveUp: (modId: string) => void;
  onMoveDown: (modId: string) => void;
  onDelete: (modId: string) => void;
  onOpenImport: () => void;
  onOpenImportModpack?: () => void;
  onOpenExportModpack?: () => void;
  onProvisionModEngine?: () => void;
}

export const ModList: React.FC<ModListProps> = ({
  mods,
  selectedModId,
  conflictModIds,
  isLoading,
  isProvisioningEngine = false,
  onSelect,
  onToggle,
  onMoveUp,
  onMoveDown,
  onDelete,
  onOpenImport,
  onOpenImportModpack,
  onOpenExportModpack,
  onProvisionModEngine,
}) => {
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<string>('all');

  const categories = useMemo(() => {
    const set = new Set<string>();
    mods.forEach((m) => {
      if (m.category) set.add(m.category);
    });
    return Array.from(set);
  }, [mods]);

  const filteredMods = useMemo(() => {
    let list = mods;
    if (selectedCategory !== 'all') {
      list = list.filter((m) => m.category === selectedCategory);
    }
    if (searchQuery.trim()) {
      const q = searchQuery.toLowerCase();
      list = list.filter(
        (m) =>
          m.name.toLowerCase().includes(q) ||
          m.id.toLowerCase().includes(q) ||
          m.author.toLowerCase().includes(q) ||
          m.tags.some((t) => t.toLowerCase().includes(q))
      );
    }
    return list;
  }, [mods, selectedCategory, searchQuery]);

  const hasModEngine = useMemo(() => {
    return mods.some(
      (m) => m.category === 'loader' || m.id.toLowerCase().includes('engine')
    );
  }, [mods]);

  return (
    <aside className="w-[420px] h-full flex flex-col border-r border-hairline-soft bg-surface-soft select-none flex-shrink-0">
      {/* Top Search & Filter Bar */}
      <div className="p-4 border-b border-hairline-soft bg-canvas space-y-3">
        {/* Search Capsule Input */}
        <div className="relative flex items-center">
          <Search className="w-4 h-4 text-steel absolute left-3.5 pointer-events-none" />
          <input
            type="text"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            placeholder="搜索模组名称、作者或标签..."
            className="w-full pl-9 pr-8 py-2 rounded-full bg-surface-soft focus:bg-canvas border border-transparent focus:border-hairline text-xs text-ink placeholder-steel outline-none transition font-sans shadow-inner"
          />
          {searchQuery && (
            <button
              type="button"
              onClick={() => setSearchQuery('')}
              className="absolute right-3 text-steel hover:text-ink"
            >
              <X className="w-3.5 h-3.5" />
            </button>
          )}
        </div>

        {/* Pill Tab Filter Chips (DESIGN.md button-pill-tab) */}
        <div className="flex items-center gap-1.5 overflow-x-auto pb-1 text-xs no-scrollbar">
          <button
            type="button"
            onClick={() => setSelectedCategory('all')}
            className={`px-3.5 py-1 rounded-full text-xs font-bold transition whitespace-nowrap ${
              selectedCategory === 'all'
                ? 'bg-ink-deep text-white shadow-sm'
                : 'bg-canvas text-charcoal border border-hairline hover:bg-surface-soft'
            }`}
          >
            全部 ({mods.length})
          </button>
          {categories.map((cat) => (
            <button
              key={cat}
              type="button"
              onClick={() => setSelectedCategory(cat)}
              className={`px-3.5 py-1 rounded-full text-xs font-bold transition whitespace-nowrap ${
                selectedCategory === cat
                  ? 'bg-ink-deep text-white shadow-sm'
                  : 'bg-canvas text-charcoal border border-hairline hover:bg-surface-soft'
              }`}
            >
              {getCategoryLabel(cat)}
            </button>
          ))}
        </div>
      </div>

      {/* Mod Engine Quick Provision Banner if missing */}
      {!hasModEngine && onProvisionModEngine && (
        <div className="mx-3.5 mt-3 p-3 rounded-2xl bg-amber-50 border border-amber-200/80 flex items-center justify-between gap-3 shadow-subtle flex-shrink-0">
          <div className="space-y-0.5 min-w-0">
            <div className="flex items-center gap-1.5 text-xs font-bold text-amber-950">
              <Zap className="w-3.5 h-3.5 text-attention fill-attention" />
              <span>缺少核心引擎 (Mod Engine)</span>
            </div>
            <p className="text-[11px] text-amber-900/70 truncate">
              官方前置驱动 (katalash/ModEngine)
            </p>
          </div>
          <button
            type="button"
            onClick={onProvisionModEngine}
            disabled={isProvisioningEngine}
            className="px-3 py-1.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] whitespace-nowrap flex-shrink-0 flex items-center gap-1.5 disabled:opacity-50"
            title="将 Sekiro Mod Engine 装配到模组列表"
          >
            {isProvisioningEngine ? (
              <Loader2 className="w-3.5 h-3.5 animate-spin" />
            ) : (
              <DownloadCloud className="w-3.5 h-3.5" />
            )}
            <span>装配引擎</span>
          </button>
        </div>
      )}

      {/* Mod Cards List */}
      <div className="flex-1 overflow-y-auto p-3.5 space-y-2.5">
        {filteredMods.length === 0 ? (
          <div className="h-64 flex flex-col items-center justify-center text-steel gap-3 p-6 text-center">
            <div className="w-12 h-12 rounded-full bg-white border border-hairline-soft flex items-center justify-center shadow-subtle">
              <FolderArchive className="w-6 h-6 stroke-[1.5] text-stone" />
            </div>
            <div>
              <p className="text-sm font-bold text-ink-deep mb-1">
                {mods.length === 0 ? '暂存库暂无模组' : '未检索到匹配模组'}
              </p>
              <p className="text-xs text-steel leading-relaxed max-w-[240px]">
                {mods.length === 0
                  ? '点击下方「导入 Mod」或直接装配 Mod Engine 核心'
                  : '请尝试更换搜索关键字或重置分类筛选'}
              </p>
              {mods.length === 0 && !hasModEngine && onProvisionModEngine && (
                <button
                  type="button"
                  onClick={onProvisionModEngine}
                  disabled={isProvisioningEngine}
                  className="mt-3 inline-flex items-center gap-1.5 px-4 py-1.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition"
                >
                  {isProvisioningEngine ? (
                    <Loader2 className="w-3.5 h-3.5 animate-spin" />
                  ) : (
                    <DownloadCloud className="w-3.5 h-3.5" />
                  )}
                  <span>装配 Mod Engine</span>
                </button>
              )}
            </div>
          </div>
        ) : (
          filteredMods.map((mod, idx) => {
            const isSelected = selectedModId === mod.id;
            const badgeClass = getCategoryBadgeClass(mod.category);
            const hasConflict = conflictModIds.has(mod.id);

            return (
              <div
                key={mod.id}
                onClick={() => onSelect(mod.id)}
                className={`group relative p-3.5 rounded-xxl border transition cursor-pointer flex items-start gap-3 bg-canvas ${
                  isSelected
                    ? 'border-ink-deep shadow-panel ring-1 ring-ink-deep/20'
                    : 'border-hairline-soft hover:border-hairline hover:shadow-card'
                } ${!mod.enabled ? 'opacity-65 grayscale-[30%]' : ''}`}
              >
                {/* Reorder Handles & Toggle Switch */}
                <div className="flex flex-col items-center gap-1.5 pt-0.5 flex-shrink-0">
                  {/* Switch Toggle */}
                  <button
                    type="button"
                    onClick={(e) => {
                      e.stopPropagation();
                      onToggle(mod.id, !mod.enabled);
                    }}
                    className={`w-9 h-5 rounded-full transition-colors p-0.5 flex items-center relative ${
                      mod.enabled ? 'bg-primary' : 'bg-hairline'
                    }`}
                    title={mod.enabled ? '已启用 (点击停用)' : '已停用 (点击启用)'}
                  >
                    <span
                      className={`w-4 h-4 rounded-full bg-white transition-transform transform shadow-sm ${
                        mod.enabled ? 'translate-x-4' : 'translate-x-0'
                      }`}
                    />
                  </button>

                  {/* Priority Pill */}
                  <div
                    className="mt-0.5 px-2 py-0.5 rounded-full bg-surface-soft border border-hairline-soft text-[10px] font-mono text-slate font-bold"
                    title="部署优先级数值（数值越小越优先生效）"
                  >
                    P:{mod.priority}
                  </div>

                  {/* Move Up/Down Controls */}
                  <div className="flex flex-col gap-0.5 mt-0.5 opacity-0 group-hover:opacity-100 transition">
                    <button
                      type="button"
                      onClick={(e) => {
                        e.stopPropagation();
                        onMoveUp(mod.id);
                      }}
                      disabled={idx === 0}
                      className="p-1 rounded-full hover:bg-surface-soft text-slate hover:text-ink disabled:opacity-20"
                      title="提高优先级 (上移)"
                    >
                      <ArrowUp className="w-3 h-3" />
                    </button>
                    <button
                      type="button"
                      onClick={(e) => {
                        e.stopPropagation();
                        onMoveDown(mod.id);
                      }}
                      disabled={idx === filteredMods.length - 1}
                      className="p-1 rounded-full hover:bg-surface-soft text-slate hover:text-ink disabled:opacity-20"
                      title="降低优先级 (下移)"
                    >
                      <ArrowDown className="w-3 h-3" />
                    </button>
                  </div>
                </div>

                {/* Mod Info Details */}
                <div className="flex-1 min-w-0">
                  <div className="flex items-center justify-between gap-2">
                    <h3
                      className="text-xs font-bold text-ink-deep truncate group-hover:text-primary transition"
                      title={mod.name}
                    >
                      {{ ...mod }.name}
                    </h3>
                    <span className="text-[10px] font-mono text-steel flex-shrink-0">
                      v{mod.version}
                    </span>
                  </div>

                  {/* Author Line */}
                  <div className="text-[11px] text-steel truncate mt-0.5">
                    <span className="text-stone">by</span> {mod.author}
                  </div>

                  {/* Badges Row */}
                  <div className="flex items-center flex-wrap gap-1.5 mt-2">
                    {/* Category Tag */}
                    <span
                      className={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${badgeClass.bg} ${badgeClass.text} ${badgeClass.border}`}
                    >
                      {getCategoryLabel(mod.category)}
                    </span>

                    {/* Asset Count Badge */}
                    <span
                      className="text-[10px] font-mono px-2 py-0.5 rounded-full bg-surface-soft text-slate border border-hairline-soft"
                      title="归一化资产文件数量"
                    >
                      {mod.asset_count} 资产
                    </span>

                    {/* Total Size */}
                    <span className="text-[10px] font-mono text-steel">
                      {formatBytes(mod.total_size)}
                    </span>

                    {/* Conflict Badge */}
                    {hasConflict && (
                      <span
                        className="text-[10px] font-bold px-2 py-0.5 rounded-full bg-amber-50 text-amber-900 border border-amber-300 flex items-center gap-1"
                        title="该模组存在文件冲突碰撞"
                      >
                        <AlertTriangle className="w-2.5 h-2.5 text-attention" />
                        碰撞
                      </span>
                    )}
                  </div>
                </div>

                {/* Delete Button (hover revealed) */}
                <button
                  type="button"
                  onClick={(e) => {
                    e.stopPropagation();
                    onDelete(mod.id);
                  }}
                  className="opacity-0 group-hover:opacity-100 p-1.5 rounded-full hover:bg-rose-50 text-stone hover:text-critical transition"
                  title="从暂存区物理删除"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                </button>
              </div>
            );
          })
        )}
      </div>

      {/* Bottom Persistent Action: Import & Modpack Operations */}
      <div className="p-3 border-t border-hairline-soft bg-canvas flex-shrink-0 shadow-subtle space-y-2">
        <button
          type="button"
          onClick={onOpenImport}
          className="w-full flex items-center justify-center gap-2 py-2 px-4 rounded-full bg-surface-soft hover:bg-[#e4e9ee] border border-hairline-soft text-xs font-bold text-ink-deep transition active:scale-[0.99] group shadow-sm"
        >
          <FolderInput className="w-4 h-4 text-primary group-hover:scale-110 transition" />
          <span>导入单模组 (ZIP / 7Z / 目录)</span>
        </button>

        <div className="grid grid-cols-2 gap-2">
          <button
            type="button"
            onClick={onOpenImportModpack}
            className="flex items-center justify-center gap-1.5 py-1.5 px-3 rounded-full bg-canvas hover:bg-surface-soft border border-hairline-soft text-[11px] font-bold text-charcoal hover:text-ink transition active:scale-[0.98] shadow-subtle"
            title="解包并导入 .smmpack 模组整合包"
          >
            <PackageOpen className="w-3.5 h-3.5 text-steel" />
            <span>导入整合包</span>
          </button>
          <button
            type="button"
            onClick={onOpenExportModpack}
            className="flex items-center justify-center gap-1.5 py-1.5 px-3 rounded-full bg-canvas hover:bg-surface-soft border border-hairline-soft text-[11px] font-bold text-charcoal hover:text-ink transition active:scale-[0.98] shadow-subtle"
            title="打包导出所选模组为 .smmpack 整合包"
          >
            <Boxes className="w-3.5 h-3.5 text-steel" />
            <span>导出整合包</span>
          </button>
        </div>
      </div>
    </aside>
  );
};
