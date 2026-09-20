import React from 'react';
import { X, ShieldAlert, RotateCcw } from 'lucide-react';
import { SekiroLogo } from '../layout/SekiroLogo';

interface ConfirmModalProps {
  isOpen: boolean;
  title?: string;
  subtitle?: string;
  message: string;
  detail?: string;
  confirmText?: string;
  cancelText?: string;
  isConfirming?: boolean;
  onConfirm: () => void;
  onCancel: () => void;
}

export const ConfirmModal: React.FC<ConfirmModalProps> = ({
  isOpen,
  title = '还原纯净环境确认',
  subtitle = 'VANILLA ROLLBACK CONFIRMATION',
  message,
  detail,
  confirmText = '确认还原纯净',
  cancelText = '取消',
  isConfirming = false,
  onConfirm,
  onCancel,
}) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm select-none p-4 font-sans animate-fade-in">
      <div className="w-[520px] max-w-full bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden flex flex-col">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={34} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight">
                {title}
              </h3>
              <span className="text-[10px] text-steel font-mono">{subtitle}</span>
            </div>
          </div>
          <button
            type="button"
            onClick={onCancel}
            disabled={isConfirming}
            className="w-8 h-8 rounded-full flex items-center justify-center text-steel hover:text-ink hover:bg-surface-soft transition"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Modal Body */}
        <div className="p-6 space-y-4">
          <div className="flex items-start gap-3.5">
            <div className="w-10 h-10 rounded-2xl bg-amber-50 border border-amber-200 flex items-center justify-center flex-shrink-0 text-amber-700 shadow-subtle">
              <ShieldAlert className="w-5 h-5" />
            </div>
            <div className="space-y-1.5 flex-1">
              <div className="text-xs font-bold text-ink-deep leading-relaxed">
                {message}
              </div>
              {detail && (
                <p className="text-[11px] text-steel leading-relaxed">
                  {detail}
                </p>
              )}
            </div>
          </div>

          <div className="p-3.5 rounded-2xl bg-surface-soft border border-hairline-soft text-[11px] text-charcoal space-y-1">
            <div className="font-semibold text-ink-deep flex items-center gap-1">
              <span>安全说明：</span>
            </div>
            <ul className="list-disc list-inside text-steel space-y-0.5 pl-1">
              <li>仅清理虚拟硬链接与部署清单，不影响游戏原始资产包；</li>
              <li>您的本地 Mod 暂存库资产完好无损，可随时再次一键秒级重新部署。</li>
            </ul>
          </div>
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-4 border-t border-hairline-soft bg-surface-soft flex items-center justify-end gap-3">
          <button
            type="button"
            onClick={onCancel}
            disabled={isConfirming}
            className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-canvas text-xs font-bold transition shadow-subtle"
          >
            {cancelText}
          </button>
          <button
            type="button"
            onClick={onConfirm}
            disabled={isConfirming}
            className="flex items-center gap-2 px-6 py-2.5 rounded-full bg-ink-button hover:bg-charcoal text-white text-xs font-bold shadow-sm transition active:scale-[0.98] disabled:opacity-50"
          >
            <RotateCcw className="w-4 h-4 text-warning" />
            <span>{confirmText}</span>
          </button>
        </div>
      </div>
    </div>
  );
};
