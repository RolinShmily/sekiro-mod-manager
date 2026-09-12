import React, { useState, useEffect, useMemo } from 'react';
import {
  X,
  Boxes,
  Download,
  CheckCircle2,
  AlertCircle,
  Loader2,
  FolderArchive,
  CheckSquare,
  Square,
  FileCheck,
  Check,
} from 'lucide-react';
import { SekiroLogo } from './SekiroLogo';
import { exportModpack } from '../api';
import type { ModSummary } from '../types';
import { getCategoryBadgeClass, getCategoryLabel } from '../utils/format';

interface ExportModpackModalProps {
  isOpen: boolean;
  stagingDir: string;
  mods: ModSummary[];
  onClose: () => void;
  onExportSuccess: (outputPath: string) => void;
}

export const ExportModpackModal: React.FC<ExportModpackModalProps> = ({
  isOpen,
  stagingDir,
  mods,
  onClose,
  onExportSuccess,
}) => {
  const [name, setName] = useState('只狼体验增强整合包');
  const [version, setVersion] = useState('1.0.0');
  const [author, setAuthor] = useState('Sekiro Master');
  const [description, setDescription] = useState('精心调优的只狼 Mod 整合包，包含优化模型与体验增强。');
  const [outputPath, setOutputPath] = useState('exports/sekiro_pack_v1.0.0.smmpack');
  const [isPathManuallyEdited, setIsPathManuallyEdited] = useState(false);
  const [selectedModIds, setSelectedModIds] = useState<Set<string>>(new Set());
  const [includeSource, setIncludeSource] = useState(false);
  const [isExporting, setIsExporting] = useState(false);
  const [errorMessage, setErrorMessage] = useState('');
  const [resultPath, setResultPath] = useState<string | null>(null);

  // Initialize selection to enabled mods when opened
  useEffect(() => {
    if (isOpen) {
      const enabledIds = new Set(
        mods.filter((m) => m.enabled).map((m) => m.id)
      );
      // If no enabled mods, select all
      if (enabledIds.size === 0 && mods.length > 0) {
        setSelectedModIds(new Set(mods.map((m) => m.id)));
      } else {
        setSelectedModIds(enabledIds);
      }
      setErrorMessage('');
      setResultPath(null);
      setIsPathManuallyEdited(false);
    }
  }, [isOpen, mods]);

  // Sync outputPath with name and version if not manually touched
  useEffect(() => {
    if (!isPathManuallyEdited) {
      const sanitizedName = name.trim().replace(/[\\/:*?"<>|\s]/g, '_') || 'modpack';
      const sanitizedVer = version.trim().replace(/[\\/:*?"<>|\s]/g, '_') || '1.0.0';
      setOutputPath(`exports/${sanitizedName}-v${sanitizedVer}.smmpack`);
    }
  }, [name, version, isPathManuallyEdited]);

  const selectAll = () => {
    setSelectedModIds(new Set(mods.map((m) => m.id)));
  };

  const selectEnabledOnly = () => {
    setSelectedModIds(new Set(mods.filter((m) => m.enabled).map((m) => m.id)));
  };

  const deselectAll = () => {
    setSelectedModIds(new Set());
  };

  const toggleModSelect = (id: string) => {
    setSelectedModIds((prev) => {
      const next = new Set(prev);
      if (next.has(id)) {
        next.delete(id);
      } else {
        next.add(id);
      }
      return next;
    });
  };

  if (!isOpen) return null;

  const handleExport = async () => {
    if (!name.trim()) {
      setErrorMessage('请输入整合包名称');
      return;
    }
    if (!version.trim()) {
      setErrorMessage('请输入整合包版本');
      return;
    }
    if (selectedModIds.size === 0) {
      setErrorMessage('请至少选择一个模组纳入整合包');
      return;
    }
    if (!outputPath.trim()) {
      setErrorMessage('请输入导出文件保存路径');
      return;
    }

    setErrorMessage('');
    setIsExporting(true);
    setResultPath(null);

    try {
      const modIdsArray = Array.from(selectedModIds);
      const savedPath = await exportModpack(
        stagingDir,
        modIdsArray,
        name.trim(),
        version.trim(),
        author.trim() || undefined,
        description.trim() || undefined,
        outputPath.trim(),
        includeSource
      );
      setResultPath(savedPath);
      onExportSuccess(savedPath);
    } catch (err: any) {
      setErrorMessage(String(err));
    } finally {
      setIsExporting(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4 font-sans">
      <div className="w-[720px] max-w-full max-h-[90vh] bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas flex-shrink-0">
          <div className="flex items-center gap-3">
            <SekiroLogo size={32} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                多模组整合包打包导出 (.smmpack)
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO MODPACK ARCHIVE BUILDER</span>
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

        {/* Modal Body Scrollable */}
        <div className="p-6 space-y-5 overflow-y-auto flex-1">
          {/* Metadata Grid */}
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">整合包名称:</label>
              <input
                type="text"
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="例如: 只狼体验增强整合包"
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none shadow-subtle transition font-sans"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">版本号:</label>
              <input
                type="text"
                value={version}
                onChange={(e) => setVersion(e.target.value)}
                placeholder="例如: 1.0.0"
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none shadow-subtle transition font-mono"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">整合包作者 (选填):</label>
              <input
                type="text"
                value={author}
                onChange={(e) => setAuthor(e.target.value)}
                placeholder="例如: 制作组或作者名"
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none shadow-subtle transition font-sans"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">导出文件路径 (.smmpack):</label>
              <input
                type="text"
                value={outputPath}
                onChange={(e) => {
                  setOutputPath(e.target.value);
                  setIsPathManuallyEdited(true);
                }}
                placeholder="exports/my_modpack.smmpack"
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none shadow-subtle transition font-mono"
              />
            </div>
          </div>

          {/* Description */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep">整合包描述简介 (选填):</label>
            <textarea
              value={description}
              onChange={(e) => setDescription(e.target.value)}
              rows={2}
              placeholder="简要说明该整合包包含的内容、玩法特性及注意事项..."
              className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none shadow-subtle transition font-sans resize-none"
            />
          </div>

          {/* Mod Selection Section */}
          <div className="space-y-2">
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <label className="text-xs font-bold text-ink-deep">选择纳入整合包的模组:</label>
                <span className="text-[11px] font-mono px-2 py-0.5 rounded-full bg-surface-soft border border-hairline-soft text-slate">
                  已选 {selectedModIds.size} / {mods.length}
                </span>
              </div>
              {/* Fast Selector Buttons */}
              <div className="flex items-center gap-1.5 text-xs">
                <button
                  type="button"
                  onClick={selectAll}
                  className="px-2.5 py-0.5 rounded-full bg-surface-soft hover:bg-[#dee3e9] text-charcoal border border-hairline-soft transition text-[11px] font-medium"
                >
                  全选
                </button>
                <button
                  type="button"
                  onClick={selectEnabledOnly}
                  className="px-2.5 py-0.5 rounded-full bg-surface-soft hover:bg-[#dee3e9] text-charcoal border border-hairline-soft transition text-[11px] font-medium"
                >
                  仅已启用模组
                </button>
                <button
                  type="button"
                  onClick={deselectAll}
                  className="px-2.5 py-0.5 rounded-full bg-surface-soft hover:bg-[#dee3e9] text-stone hover:text-charcoal border border-hairline-soft transition text-[11px]"
                >
                  清空
                </button>
              </div>
            </div>

            {/* Mod List Scroll Container */}
            <div className="max-h-52 overflow-y-auto rounded-xxl border border-hairline-soft bg-surface-soft/40 divide-y divide-hairline-soft/60">
              {mods.length === 0 ? (
                <div className="p-6 text-center text-xs text-steel">暂存库中暂无可用模组</div>
              ) : (
                mods.map((mod) => {
                  const isChecked = selectedModIds.has(mod.id);
                  const badgeClass = getCategoryBadgeClass(mod.category);

                  return (
                    <div
                      key={mod.id}
                      onClick={() => toggleModSelect(mod.id)}
                      className={`px-4 py-2.5 flex items-center justify-between cursor-pointer hover:bg-surface-soft transition text-xs ${
                        isChecked ? 'bg-white' : ''
                      }`}
                    >
                      <div className="flex items-center gap-3 min-w-0 flex-1">
                        <button
                          type="button"
                          className="text-primary flex-shrink-0"
                          onClick={(e) => {
                            e.stopPropagation();
                            toggleModSelect(mod.id);
                          }}
                        >
                          {isChecked ? (
                            <CheckSquare className="w-4 h-4 text-ink-button" />
                          ) : (
                            <Square className="w-4 h-4 text-stone" />
                          )}
                        </button>
                        <div className="min-w-0 flex-1">
                          <div className="flex items-center gap-2">
                            <span className="font-bold text-ink truncate">{mod.name}</span>
                            <span className="font-mono text-[10px] text-steel">v{mod.version}</span>
                          </div>
                          <div className="text-[11px] text-steel font-mono truncate">
                            {mod.id} · priority: #{mod.priority}
                          </div>
                        </div>
                      </div>

                      <div className="flex items-center gap-2 flex-shrink-0 ml-3">
                        <span
                          className={`text-[10px] font-semibold px-2 py-0.5 rounded-full border ${badgeClass.bg} ${badgeClass.text} ${badgeClass.border}`}
                        >
                          {getCategoryLabel(mod.category)}
                        </span>
                        <span
                          className={`text-[10px] px-2 py-0.5 rounded-full border ${
                            mod.enabled
                              ? 'bg-emerald-50 text-emerald-800 border-emerald-200 font-semibold'
                              : 'bg-surface-soft text-stone border-hairline-soft'
                          }`}
                        >
                          {mod.enabled ? '已启用' : '未启用'}
                        </span>
                      </div>
                    </div>
                  );
                })
              )}
            </div>
          </div>

          {/* Include Source Package Checkbox */}
          <div className="p-4 rounded-2xl border border-hairline-soft bg-canvas hover:bg-surface-soft/40 transition">
            <label className="flex items-start gap-3 cursor-pointer">
              <input
                type="checkbox"
                checked={includeSource}
                onChange={(e) => setIncludeSource(e.target.checked)}
                className="mt-0.5 w-4 h-4 rounded border-hairline text-primary focus:ring-primary/30 accent-ink-button"
              />
              <div className="space-y-1 text-xs">
                <div className="font-bold text-ink-deep flex items-center gap-1.5">
                  <FolderArchive className="w-3.5 h-3.5 text-steel" />
                  <span>附带原始下载来源包 (Include Source Packages)</span>
                </div>
                <p className="text-steel leading-relaxed text-[11px]">
                  勾选后，若所选模组具有原始下载压缩包，将统一封装入整合包中的 <code className="font-mono bg-surface-soft px-1 rounded">.smm_source/</code> 目录，便于完整归档与分发。
                </p>
              </div>
            </label>
          </div>

          {/* Error Feedback */}
          {errorMessage && (
            <div className="p-3 rounded-xl bg-rose-50 border border-rose-200 text-xs text-critical flex items-center gap-2">
              <AlertCircle className="w-4 h-4 flex-shrink-0" />
              <span>{errorMessage}</span>
            </div>
          )}

          {/* Success Feedback */}
          {resultPath && (
            <div className="p-3.5 rounded-xl bg-emerald-50 border border-emerald-200 text-xs text-emerald-900 flex items-start gap-2.5">
              <CheckCircle2 className="w-4 h-4 text-success flex-shrink-0 mt-0.5" />
              <div className="space-y-0.5 min-w-0 flex-1">
                <span className="font-bold">整合包打包成功！</span>
                <p className="font-mono text-[11px] text-emerald-800 truncate" title={resultPath}>
                  已生成 .smmpack 容器: {resultPath}
                </p>
              </div>
            </div>
          )}
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-4 border-t border-hairline-soft bg-surface-soft flex items-center justify-end gap-3 flex-shrink-0">
          <button
            type="button"
            onClick={onClose}
            className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-canvas text-xs font-bold transition shadow-subtle"
          >
            {resultPath ? '关闭' : '取消'}
          </button>
          {!resultPath ? (
            <button
              type="button"
              onClick={handleExport}
              disabled={isExporting}
              className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-40"
            >
              {isExporting ? (
                <Loader2 className="w-4 h-4 animate-spin text-white" />
              ) : (
                <Boxes className="w-4 h-4" />
              )}
              <span>{isExporting ? '整合打包中...' : '开始生成整合包'}</span>
            </button>
          ) : (
            <button
              type="button"
              onClick={onClose}
              className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98]"
            >
              <FileCheck className="w-4 h-4" />
              <span>完成</span>
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
