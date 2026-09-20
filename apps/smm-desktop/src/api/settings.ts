import type { AppSettings } from '../types';
import { mock } from './mock';
import { callRust, NATIVE } from './tauri';

export async function getSettings(): Promise<AppSettings> {
  if (!NATIVE) return mock.getSettings();
  return callRust<AppSettings>('get_settings');
}

export async function saveSettings(settings: AppSettings): Promise<AppSettings> {
  if (!NATIVE) return mock.saveSettings(settings);
  return callRust<AppSettings>('save_settings', { settings });
}