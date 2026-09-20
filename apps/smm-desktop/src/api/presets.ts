import type { ModPreset } from '../types';
import { mock } from './mock';
import { callRust, NATIVE } from './tauri';

export async function listPresets(stagingDir: string): Promise<ModPreset[]> {
  if (!NATIVE) return [];
  return callRust<ModPreset[]>('list_presets', { stagingDir });
}

export async function createPresetFromCurrent(
  stagingDir: string,
  name: string,
  description?: string
): Promise<ModPreset> {
  if (!NATIVE) return mock.createPresetFromCurrent(name, description);
  return callRust<ModPreset>('create_preset_from_current', {
    stagingDir,
    name,
    description,
  });
}

export async function savePreset(
  stagingDir: string,
  preset: ModPreset
): Promise<ModPreset> {
  if (!NATIVE) return preset;
  return callRust<ModPreset>('save_preset', { stagingDir, preset });
}

export async function applyPreset(
  stagingDir: string,
  presetId: string
): Promise<ModPreset> {
  if (!NATIVE) return mock.applyPreset(presetId);
  return callRust<ModPreset>('apply_preset', { stagingDir, presetId });
}

export async function deletePreset(
  stagingDir: string,
  presetId: string
): Promise<void> {
  if (!NATIVE) return;
  await callRust<void>('delete_preset', { stagingDir, presetId });
}