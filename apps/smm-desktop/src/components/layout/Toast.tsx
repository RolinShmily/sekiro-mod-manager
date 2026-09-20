import React from 'react';
import {
  CheckCircle2,
  AlertTriangle,
  XCircle,
  Info,
  X,
} from 'lucide-react';
import type { ToastMessage } from '../../types';

interface ToastProps {
  toasts: ToastMessage[];
  onDismiss: (id: string) => void;
}

export const Toast: React.FC<ToastProps> = ({ toasts, onDismiss }) => {
  return (
    <div className="fixed bottom-6 right-6 z-50 flex flex-col gap-3 pointer-events-none select-none max-w-md w-full">
      {toasts.map((toast) => {
        let borderClass = 'border-hairline-soft shadow-float';
        let icon = <Info className="w-5 h-5 text-primary mt-0.5 flex-shrink-0" />;

        if (toast.type === 'success') {
          borderClass = 'border-emerald-200 shadow-[0_8px_30px_rgba(49,162,76,0.12)]';
          icon = <CheckCircle2 className="w-5 h-5 text-success mt-0.5 flex-shrink-0" />;
        } else if (toast.type === 'error') {
          borderClass = 'border-rose-200 shadow-[0_8px_30px_rgba(228,30,63,0.12)]';
          icon = <XCircle className="w-5 h-5 text-critical mt-0.5 flex-shrink-0" />;
        } else if (toast.type === 'warning') {
          borderClass = 'border-amber-200 shadow-[0_8px_30px_rgba(247,185,40,0.12)]';
          icon = <AlertTriangle className="w-5 h-5 text-attention mt-0.5 flex-shrink-0" />;
        }

        return (
          <div
            key={toast.id}
            className={`pointer-events-auto w-full p-4 rounded-xxl border bg-canvas/95 backdrop-blur transition-all ${borderClass}`}
          >
            <div className="flex items-start justify-between gap-3">
              <div className="flex items-start gap-3 flex-1 min-w-0">
                {icon}
                <div className="space-y-1 flex-1 min-w-0">
                  <h4 className="text-xs font-bold text-ink-deep tracking-tight font-sans">
                    {toast.title}
                  </h4>
                  <p className="text-xs text-charcoal font-sans leading-relaxed">
                    {toast.message}
                  </p>

                  {/* Deployment Metrics Highlight Card */}
                  {toast.stats && (
                    <div className="mt-2.5 p-2.5 rounded-xl bg-surface-soft border border-hairline-soft grid grid-cols-3 gap-2 text-center text-xs font-mono">
                      <div className="space-y-0.5">
                        <div className="text-[10px] text-steel uppercase font-semibold">
                          硬链接创建
                        </div>
                        <div className="text-success font-bold text-xs">
                          {toast.stats.links} 个
                        </div>
                      </div>
                      <div className="space-y-0.5">
                        <div className="text-[10px] text-steel uppercase font-semibold">
                          执行耗时
                        </div>
                        <div className="text-ink-deep font-bold text-xs">
                          {toast.stats.durationMs}ms
                        </div>
                      </div>
                      <div className="space-y-0.5">
                        <div className="text-[10px] text-steel uppercase font-semibold">
                          空间节约
                        </div>
                        <div className="text-primary font-bold text-xs">
                          {toast.stats.savedBytes || '100%'}
                        </div>
                      </div>
                    </div>
                  )}
                </div>
              </div>

              <button
                type="button"
                onClick={() => onDismiss(toast.id)}
                className="p-1 rounded-full text-stone hover:text-ink hover:bg-surface-soft transition"
              >
                <X className="w-4 h-4" />
              </button>
            </div>
          </div>
        );
      })}
    </div>
  );
};
