import React, { useEffect, useState } from 'react';
import { AlertCircle, Globe, Loader2, Pencil, X } from 'lucide-react';
import type { ModDetailsPayload, ModInfo } from '../../types';
import { updateModInfo } from '../../api';
import { SekiroLogo } from '../layout/SekiroLogo';

interface ModEditMetadataModalProps {
  open: boolean;
  stagingDir: string;
  details: ModDetailsPayload;
  onClose: () => void;
  onSaved?: (updated: ModInfo) => void;
  onShowToast?: (toast: {
    type: 'success' | 'error' | 'warning' | 'info';
    title: string;
    message: string;
  }) => void;
}

/** 编辑模组元数据与来源信息弹窗。 */
export const ModEditMetadataModal: React.FC<ModEditMetadataModalProps> = ({
  open,
  stagingDir,
  details,
  onClose,
  onSaved,
  onShowToast,
}) => {
  const [isSaving, setIsSaving] = useState(false);
  const [error, setError] = useState('');
  const [form, setForm] = useState({
    name: '',
    version: '',
    author: '',
    category: '',
    source_url: '',
    description: '',
  });

  // Sync form whenever the modal opens or details change.
  useEffect(() => {
    if (!open) return;
    setForm({
      name: details.info.name || '',
      version: details.info.version || '',
      author: details.info.author || '',
      category: details.info.category || '',
      source_url: details.info.source_url || details.info.homepage || '',
      description: details.info.description || '',
    });
    setError('');
  }, [details, open]);

  if (!open) return null;

  const handleSave = async () => {
    if (!form.name.trim()) {
      setError('模组名称不能为空');
      return;
    }
    if (!form.version.trim()) {
      setError('版本号不能为空');
      return;
    }

    setError('');
    setIsSaving(true);

    try {
      const updatedPatch: ModInfo = {
        ...details.info,
        name: form.name.trim(),
        version: form.version.trim(),
        author: form.author.trim() || 'Unknown',
        category: form.category.trim() || details.info.category,
        source_url: form.source_url.trim() || null,
        homepage: null,
        description: form.description.trim() || null,
      };

      const result = await updateModInfo(stagingDir, details.info.id, updatedPatch);
      onClose();
      onSaved?.(result);
      onShowToast?.({
        type: 'success',
        title: '模组元数据已保存',
        message: `已更新 '${result.name}' 的来源地址与作者信息。`,
      });
    } catch (err: any) {
      setError(String(err));
    } finally {
      setIsSaving(false);
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
                编辑模组元数据与来源信息
              </h3>
              <span className="text-[10px] text-steel font-mono">
                MOD METADATA & UPSTREAM EDITOR
              </span>
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
        <div className="p-6 space-y-4 overflow-y-auto max-h-[75vh]">
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">模组名称:</label>
              <input
                type="text"
                value={form.name}
                onChange={(e) => setForm({ ...form, name: e.target.value })}
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">版本号:</label>
              <input
                type="text"
                value={form.version}
                onChange={(e) => setForm({ ...form, version: e.target.value })}
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">作者:</label>
              <input
                type="text"
                value={form.author}
                onChange={(e) => setForm({ ...form, author: e.target.value })}
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans"
              />
            </div>

            <div className="space-y-1.5">
              <label className="text-xs font-bold text-ink-deep">分类:</label>
              <input
                type="text"
                value={form.category}
                onChange={(e) => setForm({ ...form, category: e.target.value })}
                className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono"
              />
            </div>
          </div>

          {/* Source URL Field */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep flex items-center justify-between">
              <span>模组来源链接 (URL):</span>
              <span className="text-[10px] text-steel font-mono">
                支持 3DM / Nexus / GitHub / GameBanana 等外链
              </span>
            </label>
            <div className="relative flex items-center">
              <Globe className="w-4 h-4 text-steel absolute left-3 pointer-events-none" />
              <input
                type="text"
                value={form.source_url}
                onChange={(e) => setForm({ ...form, source_url: e.target.value })}
                placeholder="例如: 3DM、Nexus Mods、GitHub 或 GameBanana 链接"
                className="w-full pl-9 pr-4 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-mono shadow-subtle"
              />
            </div>
          </div>

          {/* Description Field */}
          <div className="space-y-1.5">
            <label className="text-xs font-bold text-ink-deep">详细描述:</label>
            <textarea
              value={form.description}
              onChange={(e) => setForm({ ...form, description: e.target.value })}
              rows={3}
              placeholder="填写关于该模组的详细说明、安装指导与特性..."
              className="w-full px-3.5 py-2 rounded-xl bg-canvas border border-hairline focus:border-primary text-xs text-ink outline-none font-sans resize-none"
            />
          </div>

          {error && (
            <div className="p-3 rounded-xl bg-rose-50 border border-rose-200 text-xs text-critical flex items-center gap-2">
              <AlertCircle className="w-4 h-4 flex-shrink-0" />
              <span>{error}</span>
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
            取消
          </button>
          <button
            type="button"
            onClick={handleSave}
            disabled={isSaving}
            className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-40"
          >
            {isSaving ? (
              <Loader2 className="w-4 h-4 animate-spin text-white" />
            ) : (
              <Pencil className="w-4 h-4" />
            )}
            <span>{isSaving ? '保存中...' : '保存元数据'}</span>
          </button>
        </div>
      </div>
    </div>
  );
};