import React, { useState, useEffect } from 'react';
import {
  X,
  Archive,
  Download,
  CheckCircle2,
  AlertCircle,
  Loader2,
  FolderArchive,
  FileCheck,
} from 'lucide-react';
import { SekiroLogo } from './SekiroLogo';
import { exportSingleMod } from '../api';
import type { ModInfo } from '../types';

interface ExportModModalProps {
  isOpen: boolean;
  stagingDir: string;
  mod: ModInfo | null;
  onClose: () => void;
  onExportSuccess: (outputPath: string) => void;
}

export const ExportModModal: React.FC<ExportModModalProps> = ({
  isOpen,
  stagingDir,
  mod,
  onClose,
  onExportSuccess,
}) => {
  const [outputPath, setOutputPath] = useState('');
  const [includeSource, setIncludeSource] = useState(false);
  const [isExporting, setIsExporting] = useState(false);
  const [errorMessage, setErrorMessage] = useState('');
  const [resultPath, setResultPath] = useState<string | null>(null);

  useEffect(() => {
    if (mod) {
      const sanitizedId = mod.id.replace(/[\\/:*?"<>|]/g, '_');
      const sanitizedVer = mod.version.replace(/[\\/:*?"<>|]/g, '_');
      setOutputPath(`exports/${sanitizedId}-v${sanitizedVer}.zip`);
      setIncludeSource(false);
      setErrorMessage('');
      setResultPath(null);
    }
  }, [mod, isOpen]);

  if (!isOpen || !mod) return null;

  const handleExport = async () => {
    if (!outputPath.trim()) {
      setErrorMessage('请输入有效的导出文件路径');
      return;
    }
    setErrorMessage('');
    setIsExporting(true);
    setResultPath(null);

    try {
      const savedPath = await exportSingleMod(
        stagingDir,
        mod.id,
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
      <div className="w-[580px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={32} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                导出单模组标准包
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO MOD STANDALONE EXPORTER</span>
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
          {/* Target Mod Preview Card */}
          <div className="p-4 rounded-xxl bg-surface-soft border border-hairline-soft flex items-center justify-between">
            <div className="space-y-1 min-w-0 flex-1">
              <div className="flex items-center gap-2">
                <span className="font-bold text-sm text-ink-deep truncate">{mod.name}</span>
                <span className="px-2.5 py-0.5 rounded-full bg-canvas font-mono text-[11px] font-bold border border-hairline-soft text-slate">
                  v{mod.version}
                </span>
              </div>
              <div className="text-xs text-steel font-mono truncate">
                <span className="text-stone">标识符:</span> {mod.id} · <span className="text-stone">作者:</span> {mod.author}
              </div>
            </div>
            <Archive className="w-6 h-6 text-primary flex-shrink-0 ml-3" />
          </div>

          {/* Output Path Input */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep flex items-center justify-between">
              <span>导出归档保存路径 (.zip):</span>
              <span className="text-[10px] text-steel font-mono font-normal">ZIP 格式标准归档</span>
            </label>
            <div className="flex items-center gap-2">
              <input
                type="text"
                value={outputPath}
                onChange={(e) => {
                  setOutputPath(e.target.value);
                  if (errorMessage) setErrorMessage('');
                }}
                placeholder="例如: exports/my_mod.zip"
                className="flex-1 px-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
              />
            </div>
            <p className="text-[11px] text-steel">
              默认输出到项目或工作区的 exports 目录，将包含归一化 assets 与 mod.json。
            </p>
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
                  <span>附带原始来源包 (Include Source Package)</span>
                </div>
                <p className="text-steel leading-relaxed text-[11px]">
                  若该模组入库时在 <code className="font-mono bg-surface-soft px-1 rounded">.smm_source/</code> 中保留了原始下载包（ZIP/7Z/RAR），勾选后将一同打包至归档中，用于完整溯源备份。
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
                <span className="font-bold">导出成功！</span>
                <p className="font-mono text-[11px] text-emerald-800 truncate" title={resultPath}>
                  已生成: {resultPath}
                </p>
              </div>
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
                <Download className="w-4 h-4" />
              )}
              <span>{isExporting ? '打包导出中...' : '开始导出'}</span>
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
