import React from 'react';
import {
  X,
  Activity,
  ShieldCheck,
  ShieldAlert,
  AlertTriangle,
  Wrench,
  Loader2,
} from 'lucide-react';
import { SekiroLogo } from '../layout/SekiroLogo';
import type { HealthReport } from '../../types';

interface DoctorModalProps {
  isOpen: boolean;
  health: HealthReport | null;
  isFixing: boolean;
  onClose: () => void;
  onFixModEngine: () => void;
}

export const DoctorModal: React.FC<DoctorModalProps> = ({
  isOpen,
  health,
  isFixing,
  onClose,
  onFixModEngine,
}) => {
  if (!isOpen) return null;

  const passCount = health?.items.filter((i) => i.status === 'Pass').length || 0;
  const warnCount = health?.items.filter((i) => i.status === 'Warning').length || 0;
  const failCount = health?.items.filter((i) => i.status === 'Fail').length || 0;

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm select-none p-4">
      <div className="w-[780px] max-w-full max-h-[85vh] flex flex-col bg-canvas border border-hairline-soft rounded-xxxl shadow-float overflow-hidden">
        {/* Modal Header */}
        <div className="px-6 py-4 border-b border-hairline-soft flex items-center justify-between bg-canvas">
          <div className="flex items-center gap-3">
            <SekiroLogo size={32} className="shadow-subtle rounded-full" />
            <div>
              <h3 className="font-extrabold text-ink-deep text-sm tracking-tight font-sans">
                只狼游戏环境与 ModEngine 全景诊断
              </h3>
              <span className="text-[10px] text-steel font-mono">SEKIRO ENVIRONMENT HEALTH DOCTOR</span>
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

        {/* Health Score Summary Banner */}
        <div className="px-6 py-4 border-b border-hairline-soft bg-surface-soft flex items-center justify-between flex-shrink-0">
          <div className="space-y-1">
            <div className="text-xs text-steel font-semibold">综合健康评级</div>
            <div className="flex items-center gap-2.5">
              <span
                className={`text-sm font-bold font-mono px-3 py-0.5 rounded-full border flex items-center gap-1.5 ${
                  health?.overall_status === 'Healthy'
                    ? 'bg-emerald-50 text-emerald-800 border-emerald-300'
                    : health?.overall_status === 'Degraded'
                    ? 'bg-amber-50 text-amber-900 border-amber-300'
                    : 'bg-rose-50 text-rose-800 border-rose-300'
                }`}
              >
                {health?.overall_status === 'Healthy' ? (
                  <ShieldCheck className="w-3.5 h-3.5 text-emerald-600" />
                ) : health?.overall_status === 'Degraded' ? (
                  <Activity className="w-3.5 h-3.5 text-amber-600" />
                ) : (
                  <ShieldAlert className="w-3.5 h-3.5 text-rose-600" />
                )}
                <span>{health?.overall_status || '未知'}</span>
              </span>
              <span className="text-xs text-steel font-mono">
                (通过: {passCount} · 警告: {warnCount} · 失败: {failCount})
              </span>
            </div>
          </div>

          {/* Quick Fix Button (DESIGN.md button-buy-cta cobalt pill) */}
          <button
            type="button"
            onClick={onFixModEngine}
            disabled={isFixing}
            className="flex items-center gap-2 px-5 py-2.5 rounded-full bg-primary hover:bg-primary-deep text-white text-xs font-bold shadow-sm transition active:scale-[0.99] disabled:opacity-40"
          >
            {isFixing ? (
              <Loader2 className="w-4 h-4 animate-spin text-white" />
            ) : (
              <Wrench className="w-4 h-4" />
            )}
            <span>一键装配 / 修复 ModEngine</span>
          </button>
        </div>

        {/* Diagnostic Items List */}
        <div className="flex-1 overflow-y-auto p-6 space-y-3 bg-canvas">
          {!health || health.items.length === 0 ? (
            <div className="text-center py-12 text-steel text-xs font-sans">
              暂无诊断数据，请检查路径配置。
            </div>
          ) : (
            health.items.map((item, idx) => (
              <div
                key={idx}
                className="p-4 rounded-xxl border border-hairline-soft bg-canvas space-y-2.5 shadow-subtle"
              >
                <div className="flex items-center justify-between">
                  <div className="flex items-center gap-2.5">
                    {item.status === 'Pass' && (
                      <ShieldCheck className="w-4 h-4 text-success flex-shrink-0" />
                    )}
                    {item.status === 'Warning' && (
                      <AlertTriangle className="w-4 h-4 text-attention flex-shrink-0" />
                    )}
                    {item.status === 'Fail' && (
                      <ShieldAlert className="w-4 h-4 text-critical flex-shrink-0" />
                    )}

                    <span className="text-xs font-bold text-ink-deep font-sans">
                      {item.name}
                    </span>
                    <span className="text-[10px] font-mono px-2 py-0.5 rounded-full bg-surface-soft text-slate border border-hairline-soft">
                      {item.category}
                    </span>
                  </div>

                  <span
                    className={`text-[10px] font-mono font-bold uppercase px-2.5 py-0.5 rounded-full border ${
                      item.status === 'Pass'
                        ? 'bg-emerald-50 text-emerald-800 border-emerald-200'
                        : item.status === 'Warning'
                        ? 'bg-amber-50 text-amber-900 border-amber-300'
                        : 'bg-rose-50 text-rose-800 border-rose-200'
                    }`}
                  >
                    {item.status}
                  </span>
                </div>

                <p className="text-xs text-charcoal font-sans pl-6.5 leading-relaxed">
                  {item.message}
                </p>

                {item.remediation && (
                  <div className="ml-6.5 p-3 rounded-xl bg-amber-50 border border-amber-200 text-xs text-amber-950 font-sans flex items-start gap-2">
                    <Wrench className="w-3.5 h-3.5 text-amber-700 mt-0.5 flex-shrink-0" />
                    <div>
                      <span className="font-bold">修复建议: </span>
                      {item.remediation}
                    </div>
                  </div>
                )}
              </div>
            ))
          )}
        </div>

        {/* Modal Footer */}
        <div className="px-6 py-3.5 border-t border-hairline-soft bg-surface-soft flex items-center justify-end">
          <button
            type="button"
            onClick={onClose}
            className="px-5 py-2 rounded-full border border-hairline text-charcoal hover:text-ink hover:bg-white text-xs font-bold transition shadow-subtle"
          >
            完成
          </button>
        </div>
      </div>
    </div>
  );
};
