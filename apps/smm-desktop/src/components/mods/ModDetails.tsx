import React, { useState, useEffect } from 'react';
import {
  Archive,
  Boxes,
  Check,
  Copy,
  ExternalLink,
  FolderOpen,
  FolderTree,
  Globe,
  Pencil,
  ShieldAlert,
  Tag,
} from 'lucide-react';
import type {
  ConflictReport,
  ModDetailsPayload,
  ModInfo,
} from '../../types';
import { formatBytes, getCategoryBadgeClass, getCategoryLabel } from '../../utils/format';
import { openExternalUrl, openPathInExplorer } from '../../api';
import { ModAssetsTab } from './ModAssetsTab';
import { ModConflictsTab } from './ModConflictsTab';
import { ModEditMetadataModal } from './ModEditMetadataModal';

interface ModDetailsProps {
  stagingDir: string;
  details: ModDetailsPayload | null;
  conflicts: ConflictReport | null;
  isLoading: boolean;
  initialEditOpen?: boolean;
  onUpdateModInfo?: (updatedInfo: ModInfo) => void;
  onOpenExportMod?: (mod: ModInfo) => void;
  onShowToast?: (toast: {
    type: 'success' | 'error' | 'warning' | 'info';
    title: string;
    message: string;
  }) => void;
}

interface DomainMeta {
  platform: string;
  badgeClass: string;
  dotClass: string;
}

function parseDomainMeta(urlStr: string): DomainMeta {
  try {
    const raw =
      urlStr.startsWith('http://') || urlStr.startsWith('https://')
        ? urlStr
        : `https://${urlStr}`;
    const url = new URL(raw);
    const host = url.hostname.toLowerCase();

    if (host.includes('nexusmods.com')) {
      return {
        platform: 'Nexus Mods',
        badgeClass: 'bg-amber-100 text-amber-900 border-amber-300/80',
        dotClass: 'bg-[#da691e]',
      };
    }
    if (host.includes('3dmgame.com')) {
      return {
        platform: '3DM',
        badgeClass: 'bg-red-50 text-red-700 border-red-200',
        dotClass: 'bg-red-600',
      };
    }
    if (host.includes('gamebanana.com')) {
      return {
        platform: 'GameBanana',
        badgeClass: 'bg-yellow-50 text-yellow-800 border-yellow-300',
        dotClass: 'bg-yellow-500',
      };
    }
    if (host.includes('github.com')) {
      return {
        platform: 'GitHub',
        badgeClass: 'bg-zinc-800 text-white border-zinc-900',
        dotClass: 'bg-white',
      };
    }
    if (host.includes('pan.baidu.com') || host.includes('baidu.com')) {
      return {
        platform: '百度网盘',
        badgeClass: 'bg-blue-50 text-blue-800 border-blue-200',
        dotClass: 'bg-blue-500',
      };
    }
    if (host.includes('quark.cn')) {
      return {
        platform: '夸克网盘',
        badgeClass: 'bg-teal-50 text-teal-800 border-teal-200',
        dotClass: 'bg-teal-500',
      };
    }
    if (host.includes('bilibili.com')) {
      return {
        platform: 'Bilibili',
        badgeClass: 'bg-pink-50 text-pink-800 border-pink-200',
        dotClass: 'bg-pink-500',
      };
    }
    if (host.includes('moddb.com')) {
      return {
        platform: 'ModDB',
        badgeClass: 'bg-red-50 text-red-800 border-red-200',
        dotClass: 'bg-red-500',
      };
    }

    const cleanHost = host.replace(/^www\./, '');
    return {
      platform: cleanHost || '外链来源',
      badgeClass: 'bg-surface-soft text-slate border-hairline-soft',
      dotClass: 'bg-steel',
    };
  } catch {
    return {
      platform: '外链来源',
      badgeClass: 'bg-surface-soft text-slate border-hairline-soft',
      dotClass: 'bg-steel',
    };
  }
}

export const ModDetails: React.FC<ModDetailsProps> = ({
  stagingDir,
  details,
  conflicts,
  initialEditOpen = false,
  onUpdateModInfo,
  onOpenExportMod,
  onShowToast,
}) => {
  const [activeTab, setActiveTab] = useState<'assets' | 'conflicts'>('assets');
  const [conflictFilterScope, setConflictFilterScope] = useState<'this_mod' | 'all'>('this_mod');
  const [copiedField, setCopiedField] = useState<string | null>(null);
  const [isEditModalOpen, setIsEditModalOpen] = useState(Boolean(initialEditOpen));

  // Number of conflicts visible under the current scope (badge on the Conflicts tab).
  const relevantConflictCount = React.useMemo(() => {
    if (!conflicts || !conflicts.records) return 0;
    if (conflictFilterScope === 'all' || !details) return conflicts.records.length;
    return conflicts.records.filter(
      (r) =>
        r.winner_mod_id === details.info.id || r.shadowed_mod_ids.includes(details.info.id)
    ).length;
  }, [conflicts, details, conflictFilterScope]);

  useEffect(() => {
    if (initialEditOpen) {
      setIsEditModalOpen(true);
    }
  }, [initialEditOpen]);

  const handleOpenModFolder = async () => {
    if (!stagingDir || !details?.info.id) return;
    const cleanStaging = stagingDir.replace(/[/\\]+$/, '');
    const modPath = `${cleanStaging}/${details.info.id}`;
    try {
      await openPathInExplorer(modPath);
    } catch (e: any) {
      onShowToast?.({
        type: 'error',
        title: '打开目录失败',
        message: String(e),
      });
    }
  };

  const handleCopy = (text: string, fieldName: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(fieldName);
    onShowToast?.({
      type: 'info',
      title: '已复制到剪贴板',
      message: text,
    });
    setTimeout(() => {
      setCopiedField((cur) => (cur === fieldName ? null : cur));
    }, 2000);
  };

  const handleOpenLink = async (url: string) => {
    if (!url) return;
    try {
      await openExternalUrl(url);
    } catch (e) {
      console.error('Failed to open external url:', e);
    }
  };

  if (!details) {
    return (
      <main className="flex-1 h-full flex flex-col items-center justify-center bg-canvas select-none p-8 text-center font-sans">
        <div className="w-16 h-16 rounded-full bg-surface-soft flex items-center justify-center mb-4 border border-hairline-soft shadow-subtle">
          <Boxes className="w-8 h-8 stroke-[1.5] text-stone" />
        </div>
        <h3 className="text-base font-bold text-ink-deep mb-1">
          请从左侧选择一个模组查看深度透视
        </h3>
        <p className="text-xs text-steel font-sans max-w-sm leading-relaxed">
          支持只狼归一化资产层级树、特殊槽位标记与全景冲突遮蔽矩阵
        </p>
      </main>
    );
  }

  const categoryBadge = getCategoryBadgeClass(details.info.category);

  return (
    <main className="flex-1 h-full flex flex-col bg-canvas select-none min-w-0 overflow-hidden font-sans">
      {/* PDP Hero Header Section */}
      <div className="p-6 border-b border-hairline-soft bg-canvas flex-shrink-0 space-y-3.5">
        <div className="flex items-start justify-between gap-6">
          <div className="space-y-2 min-w-0 flex-1">
            {/* Title & Top Badges */}
            <div className="flex items-center flex-wrap gap-2.5">
              <h2 className="text-2xl font-black text-ink-deep tracking-tight font-sans truncate">
                {details.info.name}
              </h2>
              <span className="px-3 py-0.5 rounded-full bg-surface-soft text-charcoal font-mono text-xs font-bold border border-hairline-soft">
                v{details.info.version}
              </span>
              <span
                className={`text-xs font-semibold px-3 py-0.5 rounded-full border ${categoryBadge.bg} ${categoryBadge.text} ${categoryBadge.border}`}
              >
                {getCategoryLabel(details.info.category)}
              </span>
            </div>

            {/* PDP Metadata Line */}
            <div className="flex items-center flex-wrap gap-x-4 gap-y-1.5 text-xs text-steel font-sans pt-0.5">
              <div className="flex items-center gap-1">
                <span className="text-stone">作者:</span>
                <span className="text-ink font-semibold">{details.info.author}</span>
              </div>
              <div className="flex items-center gap-1 font-mono">
                <span className="text-stone">标识符:</span>
                <span className="text-slate font-medium">{details.info.id}</span>
              </div>
              <div className="flex items-center gap-1">
                <span className="text-stone">优先级:</span>
                <span className="text-primary font-bold font-mono">#{details.info.priority}</span>
              </div>
              {details.info.license && (
                <div className="flex items-center gap-1">
                  <span className="text-stone">开源协议:</span>
                  <span className="text-slate">{details.info.license}</span>
                </div>
              )}
            </div>
          </div>

          {/* Right Status Badge & Primary Actions */}
          <div className="flex flex-col items-end gap-2 flex-shrink-0">
            <span
              className={`px-3 py-1 rounded-full text-xs font-bold border flex items-center gap-1.5 ${
                details.info.enabled
                  ? 'bg-emerald-50 text-emerald-800 border-emerald-200 shadow-sm'
                  : 'bg-surface-soft text-slate border-hairline'
              }`}
            >
              <span
                className={`w-2 h-2 rounded-full ${
                  details.info.enabled ? 'bg-success' : 'bg-stone'
                }`}
              />
              {details.info.enabled ? '已激活部署' : '已停用'}
            </span>
            <div className="text-[11px] font-mono text-stone">
              共 {details.total_assets} 个资产 · {formatBytes(details.total_size)}
            </div>
          </div>
        </div>

        {/* Source URLs & Direct Operations Strip */}
        <div className="flex items-center justify-between flex-wrap gap-2.5 pt-1">
          <div className="flex items-center flex-wrap gap-2">
            {/* Unified Mod Source Chip Capsule */}
            {(() => {
              const currentSource = details.info.source_url || details.info.homepage;
              if (currentSource) {
                const meta = parseDomainMeta(currentSource);
                return (
                  <div className="flex items-center gap-1.5 pl-1.5 pr-2 py-1 rounded-full bg-white border border-hairline-soft text-xs shadow-subtle hover:border-slate/40 transition group">
                    {/* Domain Tag */}
                    <span
                      className={`flex items-center gap-1 px-2 py-0.5 rounded-full text-[10px] font-bold border ${meta.badgeClass} flex-shrink-0 select-none`}
                    >
                      <span className={`w-1.5 h-1.5 rounded-full ${meta.dotClass}`} />
                      <span>{meta.platform}</span>
                    </span>

                    {/* Truncated URL */}
                    <span
                      className="font-mono text-ink font-medium max-w-[260px] truncate text-[11px]"
                      title={currentSource}
                    >
                      {currentSource}
                    </span>

                    {/* Actions Strip */}
                    <div className="flex items-center gap-0.5 ml-0.5 border-l border-hairline-soft pl-1 text-steel">
                      <button
                        type="button"
                        onClick={() => handleOpenLink(currentSource)}
                        className="p-1 hover:bg-surface-soft rounded-full hover:text-ink transition"
                        title="在默认浏览器中打开外链"
                      >
                        <ExternalLink className="w-3.5 h-3.5" />
                      </button>
                      <button
                        type="button"
                        onClick={() => handleCopy(currentSource, 'source')}
                        className="p-1 hover:bg-surface-soft rounded-full hover:text-ink transition"
                        title="一键复制来源链接"
                      >
                        {copiedField === 'source' ? (
                          <Check className="w-3.5 h-3.5 text-success" />
                        ) : (
                          <Copy className="w-3.5 h-3.5" />
                        )}
                      </button>
                      <button
                        type="button"
                        onClick={() => setIsEditModalOpen(true)}
                        className="p-1 hover:bg-surface-soft rounded-full hover:text-ink transition"
                        title="编辑来源链接"
                      >
                        <Pencil className="w-3 h-3 text-steel group-hover:text-primary" />
                      </button>
                    </div>
                  </div>
                );
              }

              return (
                <button
                  type="button"
                  onClick={() => setIsEditModalOpen(true)}
                  className="flex items-center gap-1.5 px-3 py-1 rounded-full bg-surface-soft/60 hover:bg-surface-soft border border-dashed border-hairline hover:border-primary/50 text-xs text-stone hover:text-primary transition group shadow-subtle"
                  title="点击填写模组来源链接"
                >
                  <Globe className="w-3.5 h-3.5 text-stone group-hover:text-primary" />
                  <span className="text-[11px]">未设定来源链接</span>
                  <span className="text-primary font-bold text-[10px] ml-0.5 underline decoration-dotted">
                    + 关联来源
                  </span>
                </button>
              );
            })()}
          </div>

          {/* Action CTAs: Open Folder, Edit Metadata & Export Mod */}
          <div className="flex items-center gap-2">
            <button
              type="button"
              onClick={handleOpenModFolder}
              className="flex items-center gap-1.5 px-3.5 py-1.5 rounded-full bg-surface-soft hover:bg-[#dee3e9] border border-hairline-soft text-charcoal hover:text-ink text-xs font-bold transition shadow-subtle active:scale-[0.98]"
              title="在 Windows 资源管理器中直接打开该模组目录"
            >
              <FolderOpen className="w-3.5 h-3.5 text-steel" />
              <span>打开目录</span>
            </button>
            <button
              type="button"
              onClick={() => setIsEditModalOpen(true)}
              className="flex items-center gap-1.5 px-3.5 py-1.5 rounded-full bg-surface-soft hover:bg-[#dee3e9] border border-hairline-soft text-charcoal hover:text-ink text-xs font-bold transition shadow-subtle active:scale-[0.98]"
              title="修改来源链接、作者与版本等元数据"
            >
              <Pencil className="w-3.5 h-3.5 text-steel" />
              <span>编辑元数据</span>
            </button>
            <button
              type="button"
              onClick={() => onOpenExportMod && onOpenExportMod(details.info)}
              className="flex items-center gap-1.5 px-3.5 py-1.5 rounded-full bg-canvas hover:bg-surface-soft border border-hairline text-ink-deep hover:text-black text-xs font-bold transition shadow-subtle active:scale-[0.98]"
              title="将当前模组导出为独立 .zip 归档"
            >
              <Archive className="w-3.5 h-3.5 text-primary" />
              <span>导出模组</span>
            </button>
          </div>
        </div>

        {/* Narrative Description Box */}
        <p className="text-xs text-charcoal leading-relaxed bg-surface-soft p-3.5 rounded-xxl border border-hairline-soft font-sans">
          {details.info.description || '暂无详细描述说明。'}
        </p>

        {/* Tags Row */}
        {details.info.tags && details.info.tags.length > 0 && (
          <div className="flex items-center flex-wrap gap-1.5 pt-0.5">
            <Tag className="w-3 h-3 text-stone mr-0.5" />
            {details.info.tags.map((tag) => (
              <span
                key={tag}
                className="text-[10px] font-mono px-2.5 py-0.5 rounded-full bg-surface-soft text-slate border border-hairline-soft"
              >
                #{tag}
              </span>
            ))}
          </div>
        )}
      </div>

      {/* Pill-style Navigation Tabs */}
      <div className="border-b border-hairline-soft bg-canvas px-6 py-2.5 flex items-center justify-between flex-shrink-0">
        <div className="flex items-center gap-2">
          <button
            type="button"
            onClick={() => setActiveTab('assets')}
            className={`flex items-center gap-2 px-4 py-1.5 rounded-full text-xs font-bold transition select-none ${
              activeTab === 'assets'
                ? 'bg-ink-deep text-white shadow-sm'
                : 'bg-white border border-hairline text-charcoal hover:bg-surface-soft'
            }`}
          >
            <FolderTree className="w-3.5 h-3.5" />
            <span>归一化资产层级树</span>
            <span
              className={`text-[10px] font-mono px-1.5 py-0.2 rounded-full ${
                activeTab === 'assets' ? 'bg-white/20 text-white' : 'bg-surface-soft text-slate'
              }`}
            >
              {details.total_assets}
            </span>
          </button>

          <button
            type="button"
            onClick={() => setActiveTab('conflicts')}
            className={`flex items-center gap-2 px-4 py-1.5 rounded-full text-xs font-bold transition select-none ${
              activeTab === 'conflicts'
                ? 'bg-ink-deep text-white shadow-sm'
                : 'bg-white border border-hairline text-charcoal hover:bg-surface-soft'
            }`}
          >
            <ShieldAlert className="w-3.5 h-3.5" />
            <span>冲突透视与遮蔽矩阵</span>
            {relevantConflictCount > 0 && (
              <span className="text-[10px] font-mono px-1.5 py-0.2 rounded-full bg-attention text-white font-bold">
                {relevantConflictCount}
              </span>
            )}
          </button>
        </div>

        {/* Scope switch inside Conflict tab */}
        {activeTab === 'conflicts' && (
          <div className="flex items-center gap-1.5 text-xs font-mono">
            <span className="text-stone text-[11px]">透视范围:</span>
            <button
              type="button"
              onClick={() => setConflictFilterScope('this_mod')}
              className={`px-3 py-1 rounded-full border transition text-[11px] font-semibold ${
                conflictFilterScope === 'this_mod'
                  ? 'bg-ink-deep text-white border-ink-deep'
                  : 'bg-canvas text-charcoal border-hairline hover:bg-surface-soft'
              }`}
            >
              当前 Mod 涉及
            </button>
            <button
              type="button"
              onClick={() => setConflictFilterScope('all')}
              className={`px-3 py-1 rounded-full border transition text-[11px] font-semibold ${
                conflictFilterScope === 'all'
                  ? 'bg-ink-deep text-white border-ink-deep'
                  : 'bg-canvas text-charcoal border-hairline hover:bg-surface-soft'
              }`}
            >
              全局矩阵 ({conflicts?.total_conflicts || 0})
            </button>
          </div>
        )}
      </div>

      {/* Main Tab Content Display */}
      <div className="flex-1 overflow-y-auto p-6 space-y-6 bg-surface-soft/40">
        {activeTab === 'assets' && <ModAssetsTab assets={details.assets || []} />}
        {activeTab === 'conflicts' && (
          <ModConflictsTab conflicts={conflicts} details={details} scope={conflictFilterScope} />
        )}
      </div>

      {/* Edit Metadata Modal */}
      <ModEditMetadataModal
        open={isEditModalOpen}
        stagingDir={stagingDir}
        details={details}
        onClose={() => setIsEditModalOpen(false)}
        onSaved={onUpdateModInfo}
        onShowToast={onShowToast}
      />
    </main>
  );
};

export default ModDetails;