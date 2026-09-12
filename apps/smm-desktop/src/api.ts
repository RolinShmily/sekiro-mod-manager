import { invoke, isTauri } from '@tauri-apps/api/core';
import type {
  AppSettings,
  ConflictReport,
  DeployResult,
  HealthReport,
  ModDetailsPayload,
  ModInfo,
  ModPackManifest,
  ModSummary,
  RestoreResult,
} from './types';

// Pure clean initial state: 0 mods by default
const MOCK_MODS: ModSummary[] = [];

export async function getSettings(): Promise<AppSettings> {
  if (isTauri()) {
    return invoke<AppSettings>('get_settings');
  }
  return {
    game_dir: 'C:\\Program Files (x86)\\Steam\\steamapps\\common\\Sekiro',
    staging_dir: 'staging',
  };
}

export async function saveSettings(settings: AppSettings): Promise<AppSettings> {
  if (isTauri()) {
    return invoke<AppSettings>('save_settings', { settings });
  }
  return settings;
}

export async function listMods(stagingDir: string): Promise<ModSummary[]> {
  if (isTauri()) {
    return invoke<ModSummary[]>('list_mods', { stagingDir });
  }
  return [...MOCK_MODS];
}

export async function getModDetails(
  stagingDir: string,
  modId: string
): Promise<ModDetailsPayload> {
  if (isTauri()) {
    return invoke<ModDetailsPayload>('get_mod_details', { stagingDir, modId });
  }

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
}

export async function toggleMod(
  stagingDir: string,
  modId: string,
  enabled: boolean
): Promise<ModInfo> {
  if (isTauri()) {
    return invoke<ModInfo>('toggle_mod', { stagingDir, modId, enabled });
  }
  const mod = MOCK_MODS.find((m) => m.id === modId);
  if (mod) mod.enabled = enabled;
  return mod || MOCK_MODS[0];
}

export async function setModPriority(
  stagingDir: string,
  modId: string,
  priority: number
): Promise<ModInfo> {
  if (isTauri()) {
    return invoke<ModInfo>('set_mod_priority', { stagingDir, modId, priority });
  }
  const mod = MOCK_MODS.find((m) => m.id === modId);
  if (mod) mod.priority = priority;
  return mod || MOCK_MODS[0];
}

export async function reorderMods(
  stagingDir: string,
  orderedIds: string[]
): Promise<ModInfo[]> {
  if (isTauri()) {
    return invoke<ModInfo[]>('reorder_mods', { stagingDir, orderedIds });
  }
  orderedIds.forEach((id, idx) => {
    const m = MOCK_MODS.find((item) => item.id === id);
    if (m) m.priority = (idx + 1) * 10;
  });
  return [...MOCK_MODS];
}

export async function deleteMod(stagingDir: string, modId: string): Promise<void> {
  if (isTauri()) {
    return invoke<void>('delete_mod', { stagingDir, modId });
  }
  const idx = MOCK_MODS.findIndex((m) => m.id === modId);
  if (idx !== -1) MOCK_MODS.splice(idx, 1);
}

export async function importModFile(
  sourcePath: string,
  stagingDir: string,
  sourceUrl?: string
): Promise<ModInfo> {
  if (isTauri()) {
    return invoke<ModInfo>('import_mod_file', {
      sourcePath,
      stagingDir,
      sourceUrl: sourceUrl?.trim() || null,
    });
  }
  return {
    id: 'imported-sample',
    name: 'Imported Mod Sample',
    version: '1.0.0',
    author: 'Author',
    category: 'weapon_skin',
    description: 'Imported via mock mode',
    homepage: null,
    source_url: sourceUrl?.trim() || null,
    license: 'MIT',
    enabled: true,
    priority: 40,
    tags: ['imported'],
    root_path: null,
  };
}

export async function updateModInfo(
  stagingDir: string,
  modId: string,
  info: ModInfo
): Promise<ModInfo> {
  if (isTauri()) {
    return invoke<ModInfo>('update_mod_info', { stagingDir, modId, info });
  }
  const idx = MOCK_MODS.findIndex((m) => m.id === modId);
  if (idx !== -1) {
    MOCK_MODS[idx] = {
      ...MOCK_MODS[idx],
      ...info,
    };
    return MOCK_MODS[idx];
  }
  return info;
}

export async function exportSingleMod(
  stagingDir: string,
  modId: string,
  outputPath: string,
  includeSource: boolean
): Promise<string> {
  if (isTauri()) {
    return invoke<string>('export_single_mod', {
      stagingDir,
      modId,
      outputPath,
      includeSource,
    });
  }
  return outputPath;
}

export async function exportModpack(
  stagingDir: string,
  modIds: string[],
  name: string,
  version: string,
  author?: string,
  description?: string,
  outputPath?: string,
  includeSource?: boolean
): Promise<string> {
  const finalOutputPath =
    outputPath ||
    `exports/${name.replace(/[\\/:*?"<>|]/g, '_')}-v${version}.smmpack`;
  if (isTauri()) {
    return invoke<string>('export_modpack', {
      stagingDir,
      modIds,
      name,
      version,
      author: author || null,
      description: description || null,
      outputPath: finalOutputPath,
      includeSource: Boolean(includeSource),
    });
  }
  return finalOutputPath;
}

export async function importModpack(
  stagingDir: string,
  packPath: string,
  overwrite?: boolean
): Promise<ModPackManifest> {
  if (isTauri()) {
    return invoke<ModPackManifest>('import_modpack', {
      stagingDir,
      packPath,
      overwrite: overwrite ?? true,
    });
  }
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
}

export async function scanConflicts(stagingDir: string): Promise<ConflictReport> {
  if (isTauri()) {
    return invoke<ConflictReport>('scan_conflicts', { stagingDir });
  }
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
}

export async function deployMods(
  gameDir: string,
  stagingDir: string
): Promise<DeployResult> {
  if (isTauri()) {
    return invoke<DeployResult>('deploy_mods', { gameDir, stagingDir });
  }
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
}

export async function restoreMods(gameDir: string): Promise<RestoreResult> {
  if (isTauri()) {
    return invoke<RestoreResult>('restore_mods', { gameDir });
  }
  return {
    target_dir: `${gameDir}/mods`,
    removed_files: 15,
    removed_dirs: 3,
    duration_ms: 4,
    success: true,
    warnings: [],
  };
}

export async function diagnoseEnv(
  gameDir: string,
  stagingDir: string
): Promise<HealthReport> {
  if (isTauri()) {
    return invoke<HealthReport>('diagnose_env', { gameDir, stagingDir });
  }
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
}

export async function setupModEngine(
  gameDir: string,
  stagingDir: string
): Promise<void> {
  if (isTauri()) {
    return invoke<void>('setup_mod_engine', { gameDir, stagingDir });
  }
}

export async function provisionEngineMod(stagingDir: string): Promise<ModInfo> {
  if (isTauri()) {
    return invoke<ModInfo>('provision_engine_mod', { stagingDir });
  }
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
}

export async function pickFolder(
  title?: string,
  defaultPath?: string
): Promise<string | null> {
  if (isTauri()) {
    const res = await invoke<string | null>('pick_folder', {
      title,
      defaultPath,
    });
    return res;
  }
  return null;
}

export async function pickFile(
  title?: string,
  defaultPath?: string,
  filterName?: string,
  extensions?: string[]
): Promise<string | null> {
  if (isTauri()) {
    const res = await invoke<string | null>('pick_file', {
      title,
      defaultPath,
      filterName,
      extensions,
    });
    return res;
  }
  return null;
}

export async function openExternalUrl(url: string): Promise<void> {
  const trimmed = url.trim();
  if (!trimmed) return;
  if (isTauri()) {
    try {
      await invoke('open_external_url', { url: trimmed });
      return;
    } catch (err) {
      console.warn('Failed to open external url via tauri:', err);
    }
  }
  const target =
    trimmed.startsWith('http://') || trimmed.startsWith('https://')
      ? trimmed
      : `https://${trimmed}`;
  window.open(target, '_blank', 'noopener,noreferrer');
}
