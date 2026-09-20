import React, { useState, useEffect } from 'react';
import {
  X,
  PackageOpen,
  CheckCircle2,
  AlertCircle,
  Loader2,
  Sparkles,
  FileCheck,
  Layers,
  ArrowRight,
} from 'lucide-react';
import { SekiroLogo } from '../layout/SekiroLogo';
import { importModpack } from '../../api';
import type { ModPackManifest } from '../../types';

interface ImportModpackModalProps {
  isOpen: boolean;
  stagingDir: string;
  initialPackPath?: string;
  onClose: () => void;
  onImportSuccess: (manifest: ModPackManifest) => void;
}

export const ImportModpackModal: React.FC<ImportModpackModalProps> = ({
  isOpen,
  stagingDir,
  initialPackPath = '',
  onClose,
  onImportSuccess,
}) => {
  const [packPath, setPackPath] = useState(initialPackPath);
  const [overwrite, setOverwrite] = useState(true);
  const [isImporting, setIsImporting] = useState(false);
  const [errorMessage, setErrorMessage] = useState('');
  const [importedManifest, setImportedManifest] = useState<ModPackManifest | null>(null);

  useEffect(() => {
    if (isOpen) {
      setPackPath(initialPackPath);
      setOverwrite(true);
      setErrorMessage('');
      setImportedManifest(null);
    }
  }, [isOpen, initialPackPath]);

  if (!isOpen) return null;

  const handleImport = async () => {
    if (!packPath.trim()) {
      setErrorMessage('请输入或选择有效的 .smmpack 整合包文件路径');
      return;
    }
    setErrorMessage('');
    setIsImporting(true);
    setImportedManifest(null);

    try {
      const manifest = await importModpack(stagingDir, packPath.trim(), overwrite);
      setImportedManifest(manifest);
      onImportSuccess(manifest);
    } catch (err: any) {
      setErrorMessage(String(err));
    } finally {
      setIsImporting(false);
    }
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4 font-sans">
      <div className="w-[620px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={32} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                导入只狼模组整合包 (.smmpack)
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO MODPACK RESTORATION PIPELINE</span>
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
        <div className="p-6 space-y-4">
          {/* Information Card */}
          <div className="p-4 rounded-xxl bg-surface-soft border border-hairline-soft text-xs text-charcoal space-y-1.5 font-sans">
            <div className="flex items-center gap-1.5 text-ink-deep font-bold">
              <Sparkles className="w-3.5 h-3.5 text-primary" />
              <span>标准 SMM 整合包智能解包与纳管:</span>
            </div>
            <p className="text-steel leading-relaxed text-xs">
              <code className="text-ink font-mono bg-canvas px-1 rounded border border-hairline-soft">.smmpack</code> 是 SMM 专有的多模组分发容器。解包后将批量导入所有子模组至当前暂存库，并完整恢复原作者设定的<span className="text-ink font-semibold">优先级次序、启停状态、来源地址</span>以及可能附带的原始来源包。
            </p>
          </div>

          {/* SMMpack Path Input */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep">整合包文件路径 (.smmpack):</label>
            <input
              type="text"
              value={packPath}
              onChange={(e) => {
                setPackPath(e.target.value);
                if (errorMessage) setErrorMessage('');
              }}
              placeholder="例如: exports/sekiro_pack_v1.0.0.smmpack 或 D:\Downloads\pack.smmpack"
              className="w-full px-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
            />
          </div>

          {/* Quick Preset / Sample */}
          <div className="space-y-1.5 pt-1">
            <div className="text-xs font-semibold text-steel">快捷填充测试样本:</div>
            <div className="flex items-center flex-wrap gap-2 text-xs font-mono">
              <button
                type="button"
                onClick={() => {
                  setPackPath('exports/sekiro_pack_v1.0.0.smmpack');
                  setErrorMessage('');
                }}
                className="px-3 py-1 rounded-full bg-surface-soft hover:bg-[#dee3e9] text-charcoal border border-hairline-soft text-xs transition shadow-subtle"
              >
                exports/sekiro_pack_v1.0.0.smmpack
              </button>
            </div>
          </div>

          {/* Overwrite Option */}
          <div className="p-3.5 rounded-2xl border border-hairline-soft bg-canvas hover:bg-surface-soft/40 transition">
            <label className="flex items-center gap-2.5 cursor-pointer text-xs">
              <input
                type="checkbox"
                checked={overwrite}
                onChange={(e) => setOverwrite(e.target.checked)}
                className="w-4 h-4 rounded border-hairline text-primary focus:ring-primary/30 accent-ink-button"
              />
              <span className="font-semibold text-ink-deep">
                覆盖暂存库中已存在的同名模组 (Overwrite Existing)
              </span>
            </label>
          </div>

          {/* Target Staging */}
          <div className="text-xs font-mono text-steel flex items-center gap-1.5">
            <span>解包目标暂存库:</span>
            <span className="text-ink font-bold truncate">{stagingDir}</span>
          </div>

          {/* Error Message */}
          {errorMessage && (
            <div className="p-3 rounded-xl bg-rose-50 border border-rose-200 text-xs text-critical flex items-center gap-2">
              <AlertCircle className="w-4 h-4 flex-shrink-0" />
              <span>{errorMessage}</span>
            </div>
          )}

          {/* Success Summary */}
          {importedManifest && (
            <div className="p-4 rounded-2xl bg-emerald-50 border border-emerald-200 text-xs text-emerald-900 space-y-2">
              <div className="flex items-center gap-2 font-bold text-sm text-emerald-950">
                <CheckCircle2 className="w-4 h-4 text-success" />
                <span>整合包解包纳管成功！</span>
              </div>
              <div className="grid grid-cols-2 gap-2 text-[11px] font-mono bg-white/70 p-3 rounded-xl border border-emerald-200">
                <div>
                  <span className="text-stone">名称: </span>
                  <span className="font-bold text-ink">{importedManifest.name}</span>
                </div>
                <div>
                  <span className="text-stone">版本: </span>
                  <span className="font-bold text-ink">v{importedManifest.version}</span>
                </div>
                {importedManifest.author && (
                  <div>
                    <span className="text-stone">作者: </span>
                    <span className="text-ink">{importedManifest.author}</span>
                  </div>
                )}
                <div>
                  <span className="text-stone">模组数: </span>
                  <span className="font-bold text-primary">{importedManifest.mods.length} 个</span>
                </div>
              </div>
              {importedManifest.mods.length > 0 && (
                <div className="text-[11px] text-emerald-800 flex items-center gap-1.5 pt-0.5">
                  <Layers className="w-3.5 h-3.5 text-emerald-700" />
                  <span>已纳管并恢复优先级与来源: </span>
                  <span className="font-mono font-medium truncate">
                    {importedManifest.mods.map((m) => m.name).join(', ')}
                  </span>
                </div>
              )}
            </div>
          )}
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-4 border-t border-hairline-soft bg-surface-soft flex items-center justify-end gap-3">
          <button
            type="button"
            onClick={onClose}
            className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-canvas text-xs font-bold transition shadow-subtle"
          >
            {importedManifest ? '关闭' : '取消'}
          </button>
          {!importedManifest ? (
            <button
              type="button"
              onClick={handleImport}
              disabled={isImporting}
              className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-40"
            >
              {isImporting ? (
                <Loader2 className="w-4 h-4 animate-spin text-white" />
              ) : (
                <PackageOpen className="w-4 h-4" />
              )}
              <span>{isImporting ? '解包批量导入中...' : '开始导入整合包'}</span>
            </button>
          ) : (
            <button
              type="button"
              onClick={onClose}
              className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98]"
            >
              <FileCheck className="w-4 h-4" />
              <span>查看导入模组</span>
            </button>
          )}
        </div>
      </div>
    </div>
  );
};
