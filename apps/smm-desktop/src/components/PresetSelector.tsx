import React, { useState, useEffect, useRef } from 'react';
import {
  Bookmark,
  ChevronDown,
  Plus,
  Save,
  Trash2,
  Check,
  Sparkles,
  SlidersHorizontal,
} from 'lucide-react';
import type { ModPreset } from '../types';
import {
  listPresets,
  createPresetFromCurrent,
  applyPreset,
  deletePreset,
} from '../api';

interface PresetSelectorProps {
  stagingDir: string;
  onPresetApplied: () => void;
  showToast: (title: string, message: string, type: 'success' | 'error' | 'warning' | 'info') => void;
}

export const PresetSelector: React.FC<PresetSelectorProps> = ({
  stagingDir,
  onPresetApplied,
  showToast,
}) => {
  const [presets, setPresets] = useState<ModPreset[]>([]);
  const [activePresetId, setActivePresetId] = useState<string | null>(null);
  const [isOpen, setIsOpen] = useState(false);
  const [isCreating, setIsCreating] = useState(false);
  const [newPresetName, setNewPresetName] = useState('');
  const [newPresetDesc, setNewPresetDesc] = useState('');
  const dropdownRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    loadPresets();
  }, [stagingDir]);

  useEffect(() => {
    function handleClickOutside(e: MouseEvent) {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target as Node)) {
        setIsOpen(false);
        setIsCreating(false);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  async function loadPresets() {
    try {
      const list = await listPresets(stagingDir);
      setPresets(list);
    } catch (err) {
      console.error('Failed to load presets:', err);
    }
  }

  async function handleSelectPreset(preset: ModPreset) {
    try {
      await applyPreset(stagingDir, preset.id);
      setActivePresetId(preset.id);
      setIsOpen(false);
      showToast('方案已应用', `已成功切换为预设方案「${preset.name}」`, 'success');
      onPresetApplied();
    } catch (err) {
      showToast('切换方案失败', String(err), 'error');
    }
  }

  async function handleCreatePreset(e: React.FormEvent) {
    e.preventDefault();
    if (!newPresetName.trim()) return;
    try {
      const created = await createPresetFromCurrent(
        stagingDir,
        newPresetName.trim(),
        newPresetDesc.trim() || undefined
      );
      setPresets((prev) => [...prev, created]);
      setActivePresetId(created.id);
      setNewPresetName('');
      setNewPresetDesc('');
      setIsCreating(false);
      setIsOpen(false);
      showToast('方案已保存', `已将当前模组配置保存为「${created.name}」`, 'success');
    } catch (err) {
      showToast('保存方案失败', String(err), 'error');
    }
  }

  async function handleDeletePreset(preset: ModPreset, e: React.MouseEvent) {
    e.stopPropagation();
    try {
      await deletePreset(stagingDir, preset.id);
      setPresets((prev) => prev.filter((p) => p.id !== preset.id));
      if (activePresetId === preset.id) {
        setActivePresetId(null);
      }
      showToast('方案已删除', `已删除预设方案「${preset.name}」`, 'info');
    } catch (err) {
      showToast('删除方案失败', String(err), 'error');
    }
  }

  const activePreset = presets.find((p) => p.id === activePresetId);

  return (
    <div className="relative inline-block text-left" ref={dropdownRef}>
      <button
        onClick={() => setIsOpen(!isOpen)}
        className="flex items-center gap-2 px-3 py-1.5 rounded-lg border border-[#e2e8f0] bg-white hover:bg-slate-50 text-xs text-charcoal shadow-sm transition-all focus:outline-none"
        title="选择或切换模组启用预设方案"
      >
        <Bookmark className="w-3.5 h-3.5 text-primary" />
        <span className="text-slate-400">方案:</span>
        <span className="font-semibold text-slate-800 max-w-[120px] truncate">
          {activePreset ? activePreset.name : '当前实时配置'}
        </span>
        <ChevronDown className="w-3.5 h-3.5 text-slate-400 ml-0.5" />
      </button>

      {isOpen && (
        <div className="absolute right-0 mt-2 w-72 rounded-xl bg-white shadow-2xl border border-slate-200 z-50 overflow-hidden animate-in fade-in zoom-in-95 duration-100">
          <div className="px-3 py-2 bg-slate-50 border-b border-slate-100 flex items-center justify-between">
            <div className="flex items-center gap-1.5 text-xs font-semibold text-slate-700">
              <SlidersHorizontal className="w-3.5 h-3.5 text-primary" />
              <span>模组应用预设方案</span>
            </div>
            <span className="text-[10px] text-slate-400 font-mono">
              {presets.length} 套方案
            </span>
          </div>

          <div className="max-h-60 overflow-y-auto p-1.5 space-y-1">
            {presets.length === 0 ? (
              <div className="text-center py-4 px-2 text-xs text-slate-400">
                暂无保存的方案，可将当前配置保存为新方案
              </div>
            ) : (
              presets.map((p) => {
                const isSelected = p.id === activePresetId;
                return (
                  <div
                    key={p.id}
                    onClick={() => handleSelectPreset(p)}
                    className={`group flex items-center justify-between px-2.5 py-2 rounded-lg cursor-pointer text-xs transition-colors ${
                      isSelected
                        ? 'bg-amber-50 text-amber-900 border border-amber-200'
                        : 'hover:bg-slate-50 text-slate-700'
                    }`}
                  >
                    <div className="flex items-center gap-2 truncate flex-1 min-w-0">
                      {isSelected ? (
                        <Check className="w-3.5 h-3.5 text-amber-600 flex-shrink-0" />
                      ) : (
                        <div className="w-3.5 h-3.5 flex-shrink-0" />
                      )}
                      <div className="truncate">
                        <p className="font-medium truncate">{p.name}</p>
                        <p className="text-[10px] text-slate-400">
                          {p.mods.length} 个启用模组
                        </p>
                      </div>
                    </div>
                    <button
                      onClick={(e) => handleDeletePreset(p, e)}
                      className="opacity-0 group-hover:opacity-100 p-1 hover:text-red-600 rounded transition-opacity"
                      title="删除此方案"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>
                );
              })
            )}
          </div>

          {/* Create Preset Form */}
          {isCreating ? (
            <form
              onSubmit={handleCreatePreset}
              className="p-2.5 bg-slate-50 border-t border-slate-100 space-y-2"
            >
              <input
                type="text"
                value={newPresetName}
                onChange={(e) => setNewPresetName(e.target.value)}
                placeholder="方案名称 (如: 纯净外观套)"
                autoFocus
                className="w-full px-2.5 py-1.5 text-xs bg-white border border-slate-200 rounded-md focus:border-primary focus:outline-none"
              />
              <div className="flex items-center justify-end gap-1.5">
                <button
                  type="button"
                  onClick={() => setIsCreating(false)}
                  className="px-2.5 py-1 text-[11px] text-slate-500 hover:bg-slate-200 rounded"
                >
                  取消
                </button>
                <button
                  type="submit"
                  disabled={!newPresetName.trim()}
                  className="flex items-center gap-1 px-2.5 py-1 text-[11px] bg-primary text-white rounded font-medium disabled:opacity-50"
                >
                  <Save className="w-3 h-3" />
                  保存
                </button>
              </div>
            </form>
          ) : (
            <div className="p-1.5 bg-slate-50 border-t border-slate-100">
              <button
                onClick={() => setIsCreating(true)}
                className="w-full flex items-center justify-center gap-1.5 px-2.5 py-1.5 text-xs font-medium text-slate-700 hover:text-primary hover:bg-white rounded-lg border border-transparent hover:border-slate-200 transition-all"
              >
                <Plus className="w-3.5 h-3.5 text-primary" />
                将当前配置保存为新方案
              </button>
            </div>
          )}
        </div>
      )}
    </div>
  );
};
