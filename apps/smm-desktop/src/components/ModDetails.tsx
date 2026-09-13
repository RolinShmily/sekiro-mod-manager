import React, { useState, useMemo, useEffect } from 'react';
import {
  ExternalLink,
  Copy,
  Check,
  Pencil,
  Archive,
  Globe,
  DownloadCloud,
  ShieldCheck,
  ShieldAlert,
  FolderTree,
  FileCode,
  Lock,
  Sparkles,
  Settings as SettingsIcon,
  Tag,
  AlertTriangle,
  AlertCircle,
  Info,
  CheckCircle2,
  Boxes,
  X,
  Loader2,
  FolderOpen,
} from 'lucide-react';
import type {
  AssetEntry,
  ConflictRecord,
  ConflictReport,
  ModDetailsPayload,
  ModInfo,
} from '../types';
import { formatBytes, getCategoryBadgeClass, getCategoryLabel } from '../utils/format';
import { updateModInfo, openExternalUrl, openPathInExplorer } from '../api';
import { SekiroLogo } from './SekiroLogo';

interface ModDetailsProps {
  stagingDir: string;
  details: ModDetailsPayload | null;
  conflicts: ConflictReport | null;
  isLoading: boolean;
  initialEditOpen?: boolean;
  onUpdateModInfo?: (updatedInfo: ModInfo) => void;
  onOpenExportMod?: (mod: ModInfo) => void;
  onShowToast?: (toast: { type: 'success' | 'error' | 'warning' | 'info'; title: string; message: string }) => void;
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

interface AssetGroup {
  folder: string;
  count: number;
  totalSize: number;
  entries: AssetEntry[];
}

export const ModDetails: React.FC<ModDetailsProps> = ({
  stagingDir,
  details,
  conflicts,
  isLoading,
  initialEditOpen = false,
  onUpdateModInfo,
  onOpenExportMod,
  onShowToast,
}) => {
  const [activeTab, setActiveTab] = useState<'assets' | 'conflicts'>('assets');
  const [conflictFilterScope, setConflictFilterScope] = useState<'this_mod' | 'all'>('this_mod');
  const [copiedField, setCopiedField] = useState<string | null>(null);

  // Edit Metadata Modal State
  const [isEditModalOpen, setIsEditModalOpen] = useState(Boolean(initialEditOpen));

  useEffect(() => {
    if (initialEditOpen) {
      setIsEditModalOpen(true);
    }
  }, [initialEditOpen]);
  const [isSavingEdit, setIsSavingEdit] = useState(false);

  const handleOpenModFolder = async () => {
    if (!stagingDir || !details?.info.id) return;
    const cleanStaging = stagingDir.replace(/[/\\]+$/, '');
    const modPath = `${cleanStaging}/${details.info.id}`;
    try {
      await openPathInExplorer(modPath);
    } catch (e: any) {
      if (onShowToast) {
        onShowToast({
          type: 'error',
          title: '打开目录失败',
          message: String(e),
        });
      }
    }
  };
  const [editError, setEditError] = useState('');
  const [editForm, setEditForm] = useState({
    name: '',
    version: '',
    author: '',
    category: '',
    source_url: '',
    description: '',
  });

  // Sync edit form when details change or modal opens
  useEffect(() => {
    if (details) {
      setEditForm({
        name: details.info.name || '',
        version: details.info.version || '',
        author: details.info.author || '',
        category: details.info.category || '',
        source_url: details.info.source_url || details.info.homepage || '',
        description: details.info.description || '',
      });
      setEditError('');
    }
  }, [details, isEditModalOpen]);

  // Group assets by top-level folder
  const assetGroups = useMemo<AssetGroup[]>(() => {
    if (!details || !details.assets) return [];
    const groups: Record<string, AssetGroup> = {};

    for (const asset of details.assets) {
      const parts = asset.relative_path.split('/');
      const folder = parts.length > 1 ? parts[0] : 'root';

      if (!groups[folder]) {
        groups[folder] = {
          folder,
          count: 0,
          totalSize: 0,
          entries: [],
        };
      }
      groups[folder].count += 1;
      groups[folder].totalSize += asset.file_size;
      groups[folder].entries.push(asset);
    }

    return Object.values(groups).sort((a, b) => b.count - a.count);
  }, [details]);

  // Filter conflicts
  const relevantConflicts = useMemo<ConflictRecord[]>(() => {
    if (!conflicts || !conflicts.records) return [];
    if (!details) return conflicts.records;

    if (conflictFilterScope === 'all') {
      return conflicts.records;
    }

    const modId = details.info.id;
    return conflicts.records.filter(
      (r) => r.winner_mod_id === modId || r.shadowed_mod_ids.includes(modId)
    );
  }, [conflicts, details, conflictFilterScope]);

  function getGuidance(record: ConflictRecord): string {
    const p = record.relative_path.toLowerCase();
    if (p.includes('gameparam.parambnd.dcx')) {
      return '【只狼核心机制冲突】两款 Mod 同时修改了底层参数包 (GameParam)。后加载的 Mod 会整体覆盖前者的所有修改（包括弹刀、躯干条、数值平衡等）。如需融合，需使用 WitchyBND 提取对应 Param CSV 并做合并。';
    }
    if (p.includes('wp_a_0300')) {
      return '【主武器槽位互斥】此文件为主角默认佩刀「楔丸」。最高优先级模组外观胜出生效，被遮蔽模组外观不可见。';
    }
    if (p.includes('wp_a_0310')) {
      return '【主武器槽位互斥】此文件为主角第二把武器「不死斩」。仅胜出模组的外观及拔刀特效生效。';
    }
    if (p.includes('c0000')) {
      return '【主角体模槽位互斥】此文件为主角体模 (c0000.chrbnd.dcx)。仅胜出模组的模型生效。';
    }
    return '【常规文件覆盖】高优先级模组文件正常投影生效，低优先级被遮蔽。可按需调整排序改变胜出顺序。';
  }

  const handleCopy = (text: string, fieldName: string) => {
    navigator.clipboard.writeText(text);
    setCopiedField(fieldName);
    if (onShowToast) {
      onShowToast({
        type: 'info',
        title: '已复制到剪贴板',
        message: text,
      });
    }
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

  const handleSaveMetadata = async () => {
    if (!details) return;
    if (!editForm.name.trim()) {
      setEditError('模组名称不能为空');
      return;
    }
    if (!editForm.version.trim()) {
      setEditError('版本号不能为空');
      return;
    }

    setEditError('');
    setIsSavingEdit(true);

    try {
      const updatedPatch: ModInfo = {
        ...details.info,
        name: editForm.name.trim(),
        version: editForm.version.trim(),
        author: editForm.author.trim() || 'Unknown',
        category: editForm.category.trim() || details.info.category,
        source_url: editForm.source_url.trim() || null,
        homepage: null,
        description: editForm.description.trim() || null,
      };

      const result = await updateModInfo(stagingDir, details.info.id, updatedPatch);
      setIsEditModalOpen(false);
      if (onUpdateModInfo) {
        onUpdateModInfo(result);
      }
      if (onShowToast) {
        onShowToast({
          type: 'success',
          title: '模组元数据已保存',
          message: `已更新 '${result.name}' 的来源地址与作者信息。`,
        });
      }
    } catch (err: any) {
      setEditError(String(err));
    } finally {
      setIsSavingEdit(false);
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

      {/* Pill-style Navigation Tabs (DESIGN.md button-pill-tab) */}
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
            {relevantConflicts.length > 0 && (
              <span className="text-[10px] font-mono px-1.5 py-0.2 rounded-full bg-attention text-white font-bold">
                {relevantConflicts.length}
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
        {/* TAB 1: NORMALIZED ASSET TREE */}
        {activeTab === 'assets' && (
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

            {/* Folder Groups */}
            {assetGroups.map((group) => (
              <div
                key={group.folder}
                className="rounded-xxl border border-hairline-soft bg-canvas overflow-hidden shadow-card"
              >
                {/* Group Header */}
                <div className="px-5 py-3 bg-surface-soft border-b border-hairline-soft flex items-center justify-between text-xs font-mono">
                  <div className="flex items-center gap-2">
                    <FolderTree className="w-4 h-4 text-steel flex-shrink-0" />
                    <span className="text-ink-deep font-bold tracking-wide">{group.folder}/</span>
                    <span className="text-stone text-[11px]">({group.count} 文件)</span>
                  </div>
                  <span className="text-steel text-[11px]">{formatBytes(group.totalSize)}</span>
                </div>

                {/* Items */}
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

                          {/* CRITICAL BADGE */}
                          {asset.is_critical && (
                            <span
                              className="px-2.5 py-0.5 rounded-full text-[10px] font-bold bg-rose-50 text-critical border border-rose-200 animate-pulse flex-shrink-0 flex items-center gap-1"
                              title="核心引擎参数文件 (gameparam.parambnd.dcx) - 冲突时后覆盖者决定一切数值"
                            >
                              <AlertTriangle className="w-2.5 h-2.5 text-critical" />
                              <span>CRITICAL PARAM</span>
                            </span>
                          )}

                          {/* EXCLUSIVE SLOT BADGE */}
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
        )}

        {/* TAB 2: CONFLICTS & SHADOWING MATRIX */}
        {activeTab === 'conflicts' && (
          <div className="space-y-4">
            {relevantConflicts.length === 0 ? (
              <div className="h-56 flex flex-col items-center justify-center text-steel gap-2.5 bg-canvas rounded-xxl border border-hairline-soft shadow-subtle p-8">
                <ShieldCheck className="w-12 h-12 text-success stroke-[1.5]" />
                <div className="text-sm text-ink-deep font-bold">无检测到碰撞与遮蔽</div>
                <div className="text-xs text-steel font-sans">
                  所有模组资产均独占独立目标路径，可和谐共存。
                </div>
              </div>
            ) : (
              relevantConflicts.map((record) => {
                const isCritical = record.severity === 'critical';

                return (
                  <div
                    key={record.relative_path}
                    className={`rounded-xxl border p-5 space-y-4 transition bg-canvas shadow-card ${
                      isCritical
                        ? 'border-rose-300 ring-1 ring-rose-300/30'
                        : 'border-amber-300'
                    }`}
                  >
                    {/* Colliding Target Path Header */}
                    <div className="flex items-center justify-between gap-3">
                      <div className="flex items-center gap-2 font-mono text-xs">
                        <span className="text-steel font-medium">碰撞目标路径:</span>
                        <span className="text-ink-deep font-bold bg-surface-soft px-3 py-1 rounded-full border border-hairline-soft">
                          {record.relative_path}
                        </span>
                      </div>

                      <span
                        className={`px-3 py-0.5 rounded-full text-[10px] font-mono font-bold uppercase border ${
                          isCritical
                            ? 'bg-rose-50 text-critical border-rose-200'
                            : 'bg-amber-50 text-amber-900 border-amber-300'
                        }`}
                      >
                        {record.severity}
                      </span>
                    </div>

                    {/* Winner vs Shadowed Matrix */}
                    <div className="grid grid-cols-2 gap-3 text-xs font-mono">
                      {/* Active Winner Box */}
                      <div className="p-3.5 rounded-xl bg-emerald-50/80 border border-emerald-200 flex items-center justify-between shadow-subtle">
                        <div>
                          <div className="text-[10px] text-emerald-800 font-bold uppercase tracking-wider">
                            胜出投影 (Active Winner)
                          </div>
                          <div className="text-emerald-950 font-bold mt-1 text-xs">
                            {record.winner_mod_id}
                          </div>
                        </div>
                        <CheckCircle2 className="w-5 h-5 text-success flex-shrink-0" />
                      </div>

                      {/* Shadowed Box */}
                      <div className="p-3.5 rounded-xl bg-rose-50/80 border border-rose-200 flex items-center justify-between shadow-subtle">
                        <div>
                          <div className="text-[10px] text-rose-800 font-bold uppercase tracking-wider">
                            被遮蔽 (Shadowed / Overwritten)
                          </div>
                          <div className="text-rose-950 line-through mt-1 text-xs font-medium">
                            {record.shadowed_mod_ids.join(', ')}
                          </div>
                        </div>
                        <AlertTriangle className="w-5 h-5 text-critical flex-shrink-0" />
                      </div>
                    </div>

                    {/* Actionable Remediation Guidance */}
                    <div className="p-4 rounded-xl bg-surface-soft border border-hairline-soft text-xs space-y-1.5 font-sans">
                      <div className="flex items-center gap-1.5 text-attention font-bold text-xs">
                        <Info className="w-4 h-4" />
                        <span>只狼冲突解析与指导建议:</span>
                      </div>
                      <p className="text-charcoal leading-relaxed text-xs">
                        {getGuidance(record)}
                      </p>
                    </div>
                  </div>
                );
              })
            )}
          </div>
        )}
      </div>

      {/* Edit Metadata Modal */}
      {isEditModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4 font-sans">
          <div className="w-[620px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
            {/* Modal Header */}
            <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
              <div className="flex items-center gap-3">
                <SekiroLogo size={32} className="shadow-subtle rounded-full" />
                <div>
                  <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                    编辑模组元数据与来源信息
                  </h3>
                  <span className="text-[10px] text-steel font-mono">
                    MOD METADATA & UPSTREAM EDITOR
                  </span>
                </div>
              </div>
              <button
                type="button"
                onClick={() => setIsEditModalOpen(false)}
                className="w-8 h-8 rounded-full flex items-center justify-center text-steel hover:text-ink hover:bg-surface-soft transition"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            {/* Modal Body */}
            <div className="p-6 space-y-4 overflow-y-auto max-h-[75vh]">
              <div className="grid grid-cols-2 gap-4">
                <div className="space-y-1.5">
                  <label className="text-xs font-bold text-ink-deep">模组名称:</label>
                  <input
                    type="text"
                    value={editForm.name}
                    onChange={(e) => setEditForm({ ...editForm, name: e.target.value })}
                    className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans"
                  />
                </div>

                <div className="space-y-1.5">
                  <label className="text-xs font-bold text-ink-deep">版本号:</label>
                  <input
                    type="text"
                    value={editForm.version}
                    onChange={(e) => setEditForm({ ...editForm, version: e.target.value })}
                    className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono"
                  />
                </div>

                <div className="space-y-1.5">
                  <label className="text-xs font-bold text-ink-deep">作者:</label>
                  <input
                    type="text"
                    value={editForm.author}
                    onChange={(e) => setEditForm({ ...editForm, author: e.target.value })}
                    className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans"
                  />
                </div>

                <div className="space-y-1.5">
                  <label className="text-xs font-bold text-ink-deep">分类:</label>
                  <input
                    type="text"
                    value={editForm.category}
                    onChange={(e) => setEditForm({ ...editForm, category: e.target.value })}
                    className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono"
                  />
                </div>
              </div>

              {/* Source URL Field */}
              <div className="space-y-1.5">
                <label className="text-xs font-bold text-ink-deep flex items-center justify-between">
                  <span>模组来源链接 (URL):</span>
                  <span className="text-[10px] text-steel font-mono">支持 3DM / Nexus / GitHub / GameBanana 等外链</span>
                </label>
                <div className="relative flex items-center">
                  <Globe className="w-4 h-4 text-steel absolute left-3 pointer-events-none" />
                  <input
                    type="text"
                    value={editForm.source_url}
                    onChange={(e) => setEditForm({ ...editForm, source_url: e.target.value })}
                    placeholder="例如: 3DM、Nexus Mods、GitHub 或 GameBanana 链接"
                    className="w-full pl-9 pr-4 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono shadow-subtle"
                  />
                </div>
              </div>

              {/* Description Field */}
              <div className="space-y-1.5">
                <label className="text-xs font-bold text-ink-deep">详细描述:</label>
                <textarea
                  value={editForm.description}
                  onChange={(e) => setEditForm({ ...editForm, description: e.target.value })}
                  rows={3}
                  placeholder="填写关于该模组的详细说明、安装指导与特性..."
                  className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans resize-none"
                />
              </div>

              {editError && (
                <div className="p-3 rounded-xl bg-rose-50 border border-rose-200 text-xs text-critical flex items-center gap-2">
                  <AlertCircle className="w-4 h-4 flex-shrink-0" />
                  <span>{editError}</span>
                </div>
              )}
            </div>

            {/* Modal Footer */}
            <div className="px-6 py-4 border-t border-hairline-soft bg-surface-soft flex items-center justify-end gap-3">
              <button
                type="button"
                onClick={() => setIsEditModalOpen(false)}
                className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-canvas text-xs font-bold transition shadow-subtle"
              >
                取消
              </button>
              <button
                type="button"
                onClick={handleSaveMetadata}
                disabled={isSavingEdit}
                className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-40"
              >
                {isSavingEdit ? (
                  <Loader2 className="w-4 h-4 animate-spin text-white" />
                ) : (
                  <Pencil className="w-4 h-4" />
                )}
                <span>{isSavingEdit ? '保存中...' : '保存元数据'}</span>
              </button>
            </div>
          </div>
        </div>
      )}
    </main>
  );
};
