import type { ModInfo, ModPackManifest } from '../types';
import { mock } from './mock';
import { callRust, NATIVE } from './tauri';

export async function importModFile(
  sourcePath: string,
  stagingDir: string,
  sourceUrl?: string
): Promise<ModInfo> {
  if (!NATIVE) return mock.importModFile();
  return callRust<ModInfo>('import_mod_file', {
    sourcePath,
    stagingDir,
    sourceUrl: sourceUrl?.trim() || null,
  });
}

export async function importMergedModFiles(
  sourcePaths: string[],
  stagingDir: string,
  customName?: string,
  sourceUrl?: string
): Promise<ModInfo> {
  if (!NATIVE) return mock.importMergedModFiles(customName);
  return callRust<ModInfo>('import_merged_mod_files', {
    sourcePaths,
    stagingDir,
    customName,
    sourceUrl,
  });
}

export async function exportSingleMod(
  stagingDir: string,
  modId: string,
  outputPath: string,
  includeSource: boolean
): Promise<string> {
  if (!NATIVE) return outputPath;
  return callRust<string>('export_single_mod', {
    stagingDir,
    modId,
    outputPath,
    includeSource,
  });
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
  if (!NATIVE) return mock.exportModpack(name, version, description, finalOutputPath);
  return callRust<string>('export_modpack', {
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

export async function importModpack(
  stagingDir: string,
  packPath: string,
  overwrite?: boolean
): Promise<ModPackManifest> {
  if (!NATIVE) return mock.importModpack();
  return callRust<ModPackManifest>('import_modpack', {
    stagingDir,
    packPath,
    overwrite: overwrite ?? true,
  });
}