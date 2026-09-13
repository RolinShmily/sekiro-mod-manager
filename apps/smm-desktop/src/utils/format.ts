export function formatBytes(bytes: number): string {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return `${parseFloat((bytes / Math.pow(k, i)).toFixed(2))} ${sizes[i]}`;
}

export function formatDuration(ms: number): string {
  if (ms < 1000) return `${ms}ms`;
  return `${(ms / 1000).toFixed(2)}s`;
}

export function getCategoryLabel(category: string): string {
  const map: Record<string, string> = {
    loader: '加载器 / 核心',
    weapon_skin: '武器外观',
    character_skin: '角色外观',
    gameplay_overhaul: '玩法改版',
    ui: '用户界面',
    audio: '音效 / 原声',
    animation: '招式动作',
    vfx: '视觉特效',
    map: '地图场景',
    script: '功能脚本',
    test_sample: '测试样本',
  };
  return map[category] || category;
}

export function getCategoryBadgeClass(category: string): { bg: string; text: string; border: string } {
  switch (category) {
    case 'loader':
      return { bg: 'bg-emerald-50', text: 'text-emerald-800', border: 'border-emerald-200' };
    case 'weapon_skin':
      return { bg: 'bg-amber-50', text: 'text-amber-800', border: 'border-amber-200' };
    case 'character_skin':
      return { bg: 'bg-orange-50', text: 'text-orange-800', border: 'border-orange-200' };
    case 'gameplay_overhaul':
      return { bg: 'bg-rose-50', text: 'text-rose-800', border: 'border-rose-200' };
    case 'ui':
      return { bg: 'bg-blue-50', text: 'text-primary', border: 'border-blue-200' };
    case 'audio':
      return { bg: 'bg-purple-50', text: 'text-purple-800', border: 'border-purple-200' };
    case 'animation':
      return { bg: 'bg-indigo-50', text: 'text-indigo-800', border: 'border-indigo-200' };
    case 'vfx':
      return { bg: 'bg-cyan-50', text: 'text-cyan-800', border: 'border-cyan-200' };
    default:
      return { bg: 'bg-[#f1f4f7]', text: 'text-charcoal', border: 'border-[#dee3e9]' };
  }
}
