import React, { useState } from 'react';
import {
  X,
  FolderInput,
  AlertCircle,
  Loader2,
  Sparkles,
  DownloadCloud,
  PackageOpen,
  ArrowRight,
  FileArchive,
  FolderOpen,
  Globe,
} from 'lucide-react';
import { SekiroLogo } from '../layout/SekiroLogo';
import { pickFile, pickFiles, pickFolder } from '../../api';

interface ImportModalProps {
  isOpen: boolean;
  stagingDir: string;
  isImporting: boolean;
  initialSourcePath?: string;
  onClose: () => void;
  onImport: (sourcePath: string, sourceUrl?: string) => void;
  onImportMerged?: (sourcePaths: string[], customName?: string, sourceUrl?: string) => void;
  onRouteToModpackImport?: (packPath: string) => void;
}

export const ImportModal: React.FC<ImportModalProps> = ({
  isOpen,
  stagingDir,
  isImporting,
  initialSourcePath = '',
  onClose,
  onImport,
  onImportMerged,
  onRouteToModpackImport,
}) => {
  const [sourcePath, setSourcePath] = useState(initialSourcePath);
  const [selectedFiles, setSelectedFiles] = useState<string[]>([]);
  const [isMergeMode, setIsMergeMode] = useState(true);
  const [mergedModName, setMergedModName] = useState('');
  const [sourceUrl, setSourceUrl] = useState('');
  const [errorMessage, setErrorMessage] = useState('');

  React.useEffect(() => {
    if (isOpen && initialSourcePath) {
      setSourcePath(initialSourcePath);
    }
  }, [isOpen, initialSourcePath]);

  if (!isOpen) return null;

  const isSmmpack = sourcePath.trim().toLowerCase().endsWith('.smmpack');

  async function handleBrowseFile() {
    const selected = await pickFiles(
      '选择模组压缩包 (按住 Ctrl 或 Shift 可多选合并)',
      undefined,
      'Mod 压缩包 (*.zip, *.7z, *.rar, *.smmpack)',
      ['zip', '7z', 'rar', 'smmpack']
    );
    if (selected && selected.length > 0) {
      if (selected.length === 1) {
        setSourcePath(selected[0]);
        setSelectedFiles([]);
      } else {
        setSelectedFiles(selected);
        setSourcePath(selected.join('; '));
        // Suggest a default merged name from the first file
        const firstFile = selected[0].split(/[/\\]/).pop() || '';
        const cleanName = firstFile.replace(/\.[^/.]+$/, '').replace(/[-_]/g, ' ');
        setMergedModName(cleanName);
      }
      setErrorMessage('');
    }
  }

  async function handleBrowseFolder() {
    const selected = await pickFolder('选择已解压的模组目录');
    if (selected) {
      setSourcePath(selected);
      setSelectedFiles([]);
      setErrorMessage('');
    }
  }

  function handleImport() {
    if (selectedFiles.length > 1 && isMergeMode && onImportMerged) {
      setErrorMessage('');
      const cleanFiles = selectedFiles.map((p) => p.trim().replace(/^["']|["']$/g, ''));
      onImportMerged(cleanFiles, mergedModName.trim() || undefined, sourceUrl.trim() || undefined);
      return;
    }

    const trimmedPath = sourcePath.trim().replace(/^["']|["']$/g, '');
    if (!trimmedPath) {
      setErrorMessage('请输入或选择本地模组压缩包或文件夹路径');
      return;
    }

    // If smmpack was selected, route to modpack import
    if (trimmedPath.toLowerCase().endsWith('.smmpack') && onRouteToModpackImport) {
      onRouteToModpackImport(trimmedPath);
      return;
    }

    setErrorMessage('');
    onImport(trimmedPath, sourceUrl.trim() || undefined);
  }

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4 font-sans">
      <div className="w-[620px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={32} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                导入新模组 (智能归一化解析)
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO MOD IMPORT PIPELINE</span>
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
          {/* Intelligence Highlights Card */}
          <div className="p-4 rounded-xxl bg-surface-soft border border-hairline-soft text-xs text-charcoal space-y-1.5 font-sans">
            <div className="flex items-center gap-1.5 text-ink-deep font-bold">
              <Sparkles className="w-3.5 h-3.5 text-primary" />
              <span>智能归一化与解包引擎:</span>
            </div>
            <p className="text-steel leading-relaxed text-xs">
              支持 <span className="text-ink font-semibold">.zip / .7z / .rar</span> 压缩包及未解包目录。SMM 会利用启发式目录遍历自动剥离任意层级的嵌套外壳（如 <code className="text-ink font-mono bg-canvas px-1.5 py-0.5 rounded-md border border-hairline-soft">MOD/archive/mods/parts/</code>），精确提取标准只狼资产树并生成 <code className="text-ink font-mono bg-canvas px-1.5 py-0.5 rounded-md border border-hairline-soft">mod.json</code>。
            </p>
          </div>

          {/* SMMpack Auto-Detection Banner */}
          {isSmmpack && (
            <div className="p-4 rounded-2xl bg-amber-50 border border-amber-300 flex items-start justify-between gap-3 text-xs">
              <div className="flex items-start gap-2.5">
                <PackageOpen className="w-4 h-4 text-attention flex-shrink-0 mt-0.5" />
                <div className="space-y-1">
                  <div className="font-bold text-amber-950">
                    检测到 .smmpack 模组整合包文件！
                  </div>
                  <p className="text-amber-800 text-[11px] leading-relaxed">
                    该文件包含打包的多款模组及其依赖配置。推荐使用专门的「整合包导入」流程批量解包并恢复原优先级。
                  </p>
                </div>
              </div>
              {onRouteToModpackImport && (
                <button
                  type="button"
                  onClick={() => onRouteToModpackImport(sourcePath.trim())}
                  className="flex items-center gap-1 px-3 py-1.5 rounded-full bg-ink-button text-white text-xs font-bold shadow-sm hover:bg-charcoal transition flex-shrink-0 active:scale-[0.98]"
                >
                  <span>切换整合包导入</span>
                  <ArrowRight className="w-3.5 h-3.5" />
                </button>
              )}
            </div>
          )}

          {/* Source Path Input */}
          <div className="space-y-1.5">
            <div className="flex items-center justify-between">
              <label className="text-xs font-bold text-ink-deep">本地压缩包或目录路径:</label>
              <span className="text-[10px] text-slate-400">支持多选文件合并导入</span>
            </div>
            <div className="flex items-center gap-2">
              <input
                type="text"
                value={sourcePath}
                onChange={(e) => {
                  setSourcePath(e.target.value);
                  setSelectedFiles([]);
                  if (errorMessage) setErrorMessage('');
                }}
                placeholder="选择或输入模组压缩包 (.zip / .7z / .rar) 或解压后的目录"
                className="flex-1 px-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
              />
              <button
                type="button"
                onClick={handleBrowseFile}
                className="px-3.5 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle whitespace-nowrap"
                title="打开 Windows 资源管理器选择压缩包 (可按住 Ctrl 多选)"
              >
                <FileArchive className="w-3.5 h-3.5 text-steel" />
                <span>选择文件(多选)</span>
              </button>
              <button
                type="button"
                onClick={handleBrowseFolder}
                className="px-3.5 py-2.5 rounded-xl bg-surface-soft hover:bg-canvas border border-hairline hover:border-primary/50 text-xs font-bold text-charcoal hover:text-ink flex items-center gap-1.5 transition shadow-subtle whitespace-nowrap"
                title="打开 Windows 资源管理器选择文件夹"
              >
                <FolderOpen className="w-3.5 h-3.5 text-steel" />
                <span>选择目录</span>
              </button>
            </div>
            {errorMessage && (
              <div className="text-xs text-critical flex items-center gap-1 mt-1 font-sans">
                <AlertCircle className="w-3.5 h-3.5" />
                <span>{errorMessage}</span>
              </div>
            )}
          </div>

          {/* Multi-file Merged Import Panel */}
          {selectedFiles.length > 1 && (
            <div className="p-3.5 rounded-xl bg-cyan-50/60 border border-cyan-200 space-y-2.5">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <input
                    type="checkbox"
                    id="merge-toggle"
                    checked={isMergeMode}
                    onChange={(e) => setIsMergeMode(e.target.checked)}
                    className="w-4 h-4 rounded text-primary focus:ring-primary"
                  />
                  <label htmlFor="merge-toggle" className="text-xs font-bold text-cyan-950 cursor-pointer">
                    合并导入为一个模组 (适合 NPC Cloth Physics 等多文件模组)
                  </label>
                </div>
                <span className="text-[10px] font-mono text-cyan-700 bg-cyan-100 px-2 py-0.5 rounded-full">
                  已选 {selectedFiles.length} 个文件
                </span>
              </div>

              {isMergeMode && (
                <div className="space-y-1.5 pt-1">
                  <label className="text-[11px] font-semibold text-cyan-900">
                    合并后的模组名称:
                  </label>
                  <input
                    type="text"
                    value={mergedModName}
                    onChange={(e) => setMergedModName(e.target.value)}
                    placeholder="输入合并后的展示名称 (如: NPC Cloth Physics)"
                    className="w-full px-3 py-1.5 text-xs bg-white border border-cyan-200 rounded-lg text-ink focus:border-primary focus:outline-none"
                  />
                  <div className="max-h-20 overflow-y-auto space-y-1 text-[11px] text-cyan-800 font-mono bg-white/70 p-2 rounded border border-cyan-100">
                    {selectedFiles.map((f, i) => (
                      <div key={i} className="truncate">
                        • {f.split(/[/\\]/).pop()}
                      </div>
                    ))}
                  </div>
                </div>
              )}
            </div>
          )}

          {/* Source URL Input */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep flex items-center justify-between">
              <span>模组来源链接 (选填):</span>
              <span className="text-[10px] text-steel font-mono">导入后自动保存至模组信息</span>
            </label>
            <div className="relative flex items-center">
              <Globe className="w-4 h-4 text-steel absolute left-3.5 pointer-events-none" />
              <input
                type="text"
                value={sourceUrl}
                onChange={(e) => setSourceUrl(e.target.value)}
                placeholder="支持 3DM / Nexus / GitHub / GameBanana 等来源链接"
                className="w-full pl-9 pr-4 py-2.5 rounded-xl bg-canvas border border-hairline focus:border-primary focus:ring-1 focus:ring-primary/20 text-xs text-ink outline-none font-mono shadow-subtle transition"
              />
            </div>
          </div>

          {/* Staging Target Indicator */}
          <div className="pt-2 text-xs font-mono text-steel flex items-center gap-1.5">
            <span>将被导入至当前暂存库:</span>
            <span className="text-ink font-bold truncate">{stagingDir}</span>
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
            onClick={handleImport}
            disabled={isImporting}
            className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-40"
          >
            {isImporting ? (
              <Loader2 className="w-4 h-4 animate-spin text-white" />
            ) : isSmmpack ? (
              <PackageOpen className="w-4 h-4" />
            ) : (
              <FolderInput className="w-4 h-4" />
            )}
            <span>
              {isImporting
                ? '解析解包导入中...'
                : isSmmpack
                ? '转至整合包导入'
                : '开始归一化导入'}
            </span>
          </button>
        </div>
      </div>
    </div>
  );
};
