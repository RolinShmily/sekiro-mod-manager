export type ModCategory =
  | 'loader'
  | 'gameplay_overhaul'
  | 'weapon_skin'
  | 'character_skin'
  | 'ui'
  | 'audio'
  | 'animation'
  | 'map'
  | 'script'
  | 'test_sample'
  | string;

export interface ModInfo {
  id: string;
  name: string;
  version: string;
  author: string;
  category: string;
  description?: string | null;
  homepage?: string | null;
  source_url?: string | null;
  license?: string | null;
  enabled: boolean;
  priority: number;
  tags: string[];
  root_path?: string | null;
}

export interface ModSummary extends ModInfo {
  asset_count: number;
  total_size: number;
}

export type AssetCategory =
  | 'parts'
  | 'chr'
  | 'param'
  | 'sound'
  | 'msg'
  | 'menu'
  | 'font'
  | 'mtd'
  | 'event'
  | 'map'
  | 'script'
  | 'loader'
  | 'cutscene'
  | 'other';

export interface AssetEntry {
  relative_path: string;
  source_path: string;
  file_size: number;
  category: AssetCategory;
  is_critical: boolean;
  is_exclusive_slot: boolean;
}

export interface ModDetailsPayload {
  info: ModInfo;
  assets: AssetEntry[];
  total_size: number;
  total_assets: number;
}

export type ConflictSeverity = 'info' | 'warning' | 'critical';

export interface ConflictRecord {
  relative_path: string;
  severity: ConflictSeverity;
  winner_mod_id: string;
  shadowed_mod_ids: string[];
  message: string;
}

export interface ConflictReport {
  has_critical_conflict: boolean;
  has_warning_conflict: boolean;
  total_conflicts: number;
  records: ConflictRecord[];
}

export interface DeployResult {
  target_dir: string;
  total_files: number;
  hard_links_created: number;
  copied_files: number;
  skipped_files: number;
  failed_files: number;
  bytes_saved: number;
  duration_ms: number;
  warnings: string[];
  errors: [string, string][];
}

export interface RestoreResult {
  target_dir: string;
  removed_files: number;
  removed_dirs: number;
  duration_ms: number;
  success: boolean;
  warnings: string[];
}

export type DiagnosticStatus = 'Pass' | 'Warning' | 'Fail';
export type OverallHealth = 'Healthy' | 'Degraded' | 'ActionRequired';

export interface DiagnosticItem {
  category: string;
  name: string;
  status: DiagnosticStatus;
  message: string;
  remediation?: string | null;
}

export interface HealthReport {
  game_dir: string;
  staging_dir?: string | null;
  items: DiagnosticItem[];
  overall_status: OverallHealth;
}

export interface AppSettings {
  game_dir: string;
  staging_dir: string;
  active_modal?: string | null;
}

export interface ModPackItem {
  id: string;
  name: string;
  version: string;
  author: string;
  category: string;
  priority: number;
  enabled: boolean;
  source_url?: string | null;
  homepage?: string | null;
  description?: string | null;
}

export interface ModPackManifest {
  format_version: number;
  name: string;
  version: string;
  author?: string | null;
  description?: string | null;
  created_at?: number | null;
  mods: ModPackItem[];
}

export interface ModPresetEntry {
  mod_id: string;
  priority: number;
}

export interface ModPreset {
  id: string;
  name: string;
  description?: string | null;
  created_at: number;
  updated_at: number;
  mods: ModPresetEntry[];
}

export type ViewMode = 'compact' | 'detailed' | 'category';

export interface ToastMessage {
  id: string;
  type: 'success' | 'error' | 'warning' | 'info';
  title: string;
  message: string;
  duration?: number;
  stats?: {
    links?: number;
    durationMs?: number;
    savedBytes?: string;
  };
}
