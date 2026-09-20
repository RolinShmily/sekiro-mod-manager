import { callRust, NATIVE } from './tauri';

export async function pickFolder(
  title?: string,
  defaultPath?: string
): Promise<string | null> {
  if (!NATIVE) return null;
  return callRust<string | null>('pick_folder', { title, defaultPath });
}

export async function pickFile(
  title?: string,
  defaultPath?: string,
  filterName?: string,
  extensions?: string[]
): Promise<string | null> {
  if (!NATIVE) return null;
  return callRust<string | null>('pick_file', {
    title,
    defaultPath,
    filterName,
    extensions,
  });
}

export async function pickFiles(
  title?: string,
  defaultPath?: string,
  filterName?: string,
  extensions?: string[]
): Promise<string[] | null> {
  if (!NATIVE) return null;
  return callRust<string[] | null>('pick_files', {
    title,
    defaultPath,
    filterName,
    extensions,
  });
}

/** Opens a URL in the default browser; falls back to a new tab in plain-browser dev. */
export async function openExternalUrl(url: string): Promise<void> {
  const trimmed = url.trim();
  if (!trimmed) return;

  if (NATIVE) {
    try {
      await callRust<void>('open_external_url', { url: trimmed });
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

/** Opens a path in the system file manager (Windows Explorer). */
export async function openPathInExplorer(path: string): Promise<void> {
  const trimmed = path.trim();
  if (!trimmed) return;
  if (!NATIVE) return;
  await callRust<void>('open_path_in_explorer', { path: trimmed });
}