import React, { useMemo } from 'react';
import {
  AlertTriangle,
  CheckCircle2,
  Info,
  ShieldCheck,
} from 'lucide-react';
import type { ConflictRecord, ConflictReport, ModDetailsPayload } from '../../types';

interface ModConflictsTabProps {
  conflicts: ConflictReport | null;
  details: ModDetailsPayload;
  scope: 'this_mod' | 'all';
}

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

/** 冲突透视与遮蔽矩阵 (TAB 2)。 */
export const ModConflictsTab: React.FC<ModConflictsTabProps> = ({
  conflicts,
  details,
  scope,
}) => {
  const relevantConflicts = useMemo<ConflictRecord[]>(() => {
    if (!conflicts || !conflicts.records) return [];
    if (scope === 'all') return conflicts.records;

    const modId = details.info.id;
    return conflicts.records.filter(
      (r) => r.winner_mod_id === modId || r.shadowed_mod_ids.includes(modId)
    );
  }, [conflicts, details, scope]);

  if (relevantConflicts.length === 0) {
    return (
      <div className="h-56 flex flex-col items-center justify-center text-steel gap-2.5 bg-canvas rounded-xxl border border-hairline-soft shadow-subtle p-8">
        <ShieldCheck className="w-12 h-12 text-success stroke-[1.5]" />
        <div className="text-sm text-ink-deep font-bold">无检测到碰撞与遮蔽</div>
        <div className="text-xs text-steel font-sans">
          所有模组资产均独占独立目标路径，可和谐共存。
        </div>
      </div>
    );
  }

  return (
    <div className="space-y-4">
      {relevantConflicts.map((record) => {
        const isCritical = record.severity === 'critical';

        return (
          <div
            key={record.relative_path}
            className={`rounded-xxl border p-5 space-y-4 transition bg-canvas shadow-card ${
              isCritical ? 'border-rose-300 ring-1 ring-rose-300/30' : 'border-amber-300'
            }`}
          >
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

            <div className="grid grid-cols-2 gap-3 text-xs font-mono">
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

            <div className="p-4 rounded-xl bg-surface-soft border border-hairline-soft text-xs space-y-1.5 font-sans">
              <div className="flex items-center gap-1.5 text-attention font-bold text-xs">
                <Info className="w-4 h-4" />
                <span>只狼冲突解析与指导建议:</span>
              </div>
              <p className="text-charcoal leading-relaxed text-xs">{getGuidance(record)}</p>
            </div>
          </div>
        );
      })}
    </div>
  );
};