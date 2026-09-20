import { invoke as rustInvoke, isTauri } from '@tauri-apps/api/core';

/** True when running inside the Tauri desktop webview (vs. plain-browser dev). */
export const NATIVE = isTauri();

/** Thin wrapper over the Tauri IPC invoke with full type control. */
export function callRust<T>(cmd: string, args?: Record<string, unknown>): Promise<T> {
  return rustInvoke<T>(cmd, args);
}