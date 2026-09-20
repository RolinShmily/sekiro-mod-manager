import type {
  AppSettings,
  ConflictReport,
  DeployResult,
  HealthReport,
  ModDetailsPayload,
  ModInfo,
  ModPackManifest,
  ModPreset,
  ModSummary,
  RestoreResult,
} from '../types';

// ---------------------------------------------------------------------------
// Browser-only development fallbacks (used when NATIVE === false).
// Pure initial state: 0 mods by default.
// ---------------------------------------------------------------------------
const MOCK_MODS: ModSummary[] = [];

export const mock = {
  getSettings(): AppSettings {
    return {
      game_dir: 'C:\\Program Files (x86)\\Steam\\steamapps\\common\\Sekiro',
      staging_dir: 'staging',
    };
  },

  saveSettings(settings: AppSettings): AppSettings {
    return settings;
  },

  listMods(): ModSummary[] {
    return [...MOCK_MODS];
  },

  getModDetails(stagingDir: string, modId: string): ModDetailsPayload {
    const mod = MOCK_MODS.find((m) => m.id === modId) || MOCK_MODS[0];
    return {
      info: mod,
      total_size: mod.total_size,
      total_assets: mod.asset_count,
      assets: [
        {
          relative_path: 'parts/wp_a_0300.partsbnd.dcx',
          source_path: `${stagingDir}/${mod.id}/parts/wp_a_0300.partsbnd.dcx`,
          file_size: 1420000,
          category: 'parts',
          is_critical: false,
          is_exclusive_slot: true,
        },
        {
          relative_path: 'param/gameparam/gameparam.parambnd.dcx',
          source_path: `${stagingDir}/${mod.id}/param/gameparam/gameparam.parambnd.dcx`,
          file_size: 1470000,
          category: 'param',
          is_critical: true,
          is_exclusive_slot: false,
        },
      ],
    };
  },

  toggleMod(modId: string, enabled: boolean): ModInfo {
    const mod = MOCK_MODS.find((m) => m.id === modId);
    if (mod) mod.enabled = enabled;
    return mod || MOCK_MODS[0];
  },

  setModPriority(modId: string, priority: number): ModInfo {
    const mod = MOCK_MODS.find((m) => m.id === modId);
    if (mod) mod.priority = priority;
    return mod || MOCK_MODS[0];
  },

  reorderMods(orderedIds: string[]): ModInfo[] {
    orderedIds.forEach((id, idx) => {
      const m = MOCK_MODS.find((item) => item.id === id);
      if (m) m.priority = (idx + 1) * 10;
    });
    return [...MOCK_MODS];
  },

  deleteMod(modId: string): void {
    const idx = MOCK_MODS.findIndex((m) => m.id === modId);
    if (idx !== -1) MOCK_MODS.splice(idx, 1);
  },

  importModFile(): ModInfo {
    return {
      id: 'imported-sample',
      name: 'Imported Mod Sample',
      version: '1.0.0',
      author: 'Author',
      category: 'weapon_skin',
      description: 'Imported via mock mode',
      homepage: null,
      source_url: null,
      license: 'MIT',
      enabled: true,
      priority: 40,
      tags: ['imported'],
      root_path: null,
    };
  },

  updateModInfo(modId: string, info: ModInfo): ModInfo {
    const idx = MOCK_MODS.findIndex((m) => m.id === modId);
    if (idx !== -1) {
      MOCK_MODS[idx] = { ...MOCK_MODS[idx], ...info };
      return MOCK_MODS[idx];
    }
    return info;
  },

  exportModpack(
    _name: string,
    _version: string,
    _description: string | undefined,
    finalOutputPath: string
  ): string {
    return finalOutputPath;
  },

  importModpack(): ModPackManifest {
    return {
      format_version: 1,
      name: 'Mock Sekiro Modpack',
      version: '1.0.0',
      author: 'Mock Author',
      description: 'Mock imported pack description',
      created_at: Math.floor(Date.now() / 1000),
      mods: [
        {
          id: 'mock-pack-mod-1',
          name: 'Mock Pack Mod 1',
          version: '1.0.0',
          author: 'Mock Author',
          category: 'weapon_skin',
          priority: 10,
          enabled: true,
          source_url: 'https://nexusmods.com/sekiro/mods/1',
          homepage: 'https://nexusmods.com/sekiro/mods/1',
          description: 'Part of mock pack',
        },
      ],
    };
  },

  scanConflicts(): ConflictReport {
    return {
      has_critical_conflict: true,
      has_warning_conflict: true,
      total_conflicts: 2,
      records: [
        {
          relative_path: 'parts/wp_a_0300.partsbnd.dcx',
          severity: 'warning',
          winner_mod_id: 'kusabimaru-reaper',
          shadowed_mod_ids: ['dream-of-the-damned'],
          message:
            "Exclusive model/weapon slot collision on 'parts/wp_a_0300.partsbnd.dcx'! Mod 'kusabimaru-reaper' wins; appearance from [dream-of-the-damned] will be shadowed.",
        },
        {
          relative_path: 'param/gameparam/gameparam.parambnd.dcx',
          severity: 'critical',
          winner_mod_id: 'dream-of-the-damned',
          shadowed_mod_ids: ['kusabimaru-reaper'],
          message:
            "Critical parameter collision on 'param/gameparam/gameparam.parambnd.dcx'! Overwriting will completely discard parameters from lower-priority mods.",
        },
      ],
    };
  },

  deployMods(gameDir: string): DeployResult {
    return {
      target_dir: `${gameDir}/mods`,
      total_files: 15,
      hard_links_created: 15,
      copied_files: 0,
      skipped_files: 0,
      failed_files: 0,
      bytes_saved: 4890000,
      duration_ms: 9,
      warnings: [],
      errors: [],
    };
  },

  restoreMods(gameDir: string): RestoreResult {
    return {
      target_dir: `${gameDir}/mods`,
      removed_files: 15,
      removed_dirs: 3,
      duration_ms: 4,
      success: true,
      warnings: [],
    };
  },

  diagnoseEnv(gameDir: string, stagingDir: string): HealthReport {
    return {
      game_dir: gameDir,
      staging_dir: stagingDir,
      overall_status: 'Healthy',
      items: [
        {
          category: 'Game Directory',
          name: 'Sekiro Game Executable',
          status: 'Pass',
          message: 'sekiro.exe located and verified.',
          remediation: null,
        },
        {
          category: 'ModEngine Setup',
          name: 'dinput8.dll Injection Hook',
          status: 'Pass',
          message: 'dinput8.dll active in root game directory.',
          remediation: null,
        },
        {
          category: 'ModEngine Config',
          name: 'modengine.ini Enabled & Valid',
          status: 'Pass',
          message: 'modengine.ini present: enabled=1, modOverrideDirectory="\\mods".',
          remediation: null,
        },
        {
          category: 'Storage Architecture',
          name: 'NTFS Same-Volume Fast Hardlinks',
          status: 'Pass',
          message: 'Staging and Game target reside on the same drive volume.',
          remediation: null,
        },
      ],
    };
  },

  provisionEngineMod(): ModInfo {
    return {
      id: 'mod-engine',
      name: 'Sekiro Mod Engine',
      version: '0.1.16',
      author: 'katalash',
      description: 'DirectX 11 input wrapper and mod loader for Sekiro',
      category: 'loader',
      license: 'GPL-3.0-or-later',
      enabled: true,
      priority: 0,
      tags: ['loader', 'core'],
      source_url: 'https://github.com/katalash/ModEngine',
      homepage: 'https://github.com/katalash/ModEngine',
    };
  },

  importMergedModFiles(customName?: string): ModInfo {
    return {
      id: 'mock-merged-mod',
      name: customName || 'Mock Merged Mod',
      version: '1.0.0',
      author: 'Community',
      category: 'general',
      enabled: true,
      priority: 100,
      tags: ['merged'],
      root_path: null,
      homepage: null,
      source_url: null,
      license: null,
      description: null,
    };
  },

  createPresetFromCurrent(name: string, description?: string): ModPreset {
    const now = Math.floor(Date.now() / 1000);
    return {
      id: `preset_${now}`,
      name,
      description,
      created_at: now,
      updated_at: now,
      mods: [],
    };
  },

  applyPreset(presetId: string): ModPreset {
    return {
      id: presetId,
      name: 'Mock Preset',
      created_at: 0,
      updated_at: 0,
      mods: [],
    };
  },
};