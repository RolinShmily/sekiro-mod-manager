import type { ModDetailsPayload, ModInfo, ModSummary } from '../types';
import { mock } from './mock';
import { callRust, NATIVE } from './tauri';

export async function listMods(stagingDir: string): Promise<ModSummary[]> {
  if (!NATIVE) return mock.listMods();
  return callRust<ModSummary[]>('list_mods', { stagingDir });
}

export async function getModDetails(
  stagingDir: string,
  modId: string
): Promise<ModDetailsPayload> {
  if (!NATIVE) return mock.getModDetails(stagingDir, modId);
  return callRust<ModDetailsPayload>('get_mod_details', { stagingDir, modId });
}

export async function toggleMod(
  stagingDir: string,
  modId: string,
  enabled: boolean
): Promise<ModInfo> {
  if (!NATIVE) return mock.toggleMod(modId, enabled);
  return callRust<ModInfo>('toggle_mod', { stagingDir, modId, enabled });
}

export async function setModPriority(
  stagingDir: string,
  modId: string,
  priority: number
): Promise<ModInfo> {
  if (!NATIVE) return mock.setModPriority(modId, priority);
  return callRust<ModInfo>('set_mod_priority', { stagingDir, modId, priority });
}

export async function reorderMods(
  stagingDir: string,
  orderedIds: string[]
): Promise<ModInfo[]> {
  if (!NATIVE) return mock.reorderMods(orderedIds);
  return callRust<ModInfo[]>('reorder_mods', { stagingDir, orderedIds });
}

export async function deleteMod(stagingDir: string, modId: string): Promise<void> {
  if (!NATIVE) return mock.deleteMod(modId);
  return callRust<void>('delete_mod', { stagingDir, modId });
}

export async function updateModInfo(
  stagingDir: string,
  modId: string,
  info: ModInfo
): Promise<ModInfo> {
  if (!NATIVE) return mock.updateModInfo(modId, info);
  return callRust<ModInfo>('update_mod_info', { stagingDir, modId, info });
}