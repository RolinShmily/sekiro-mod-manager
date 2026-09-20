import type {
  ConflictReport,
  DeployResult,
  HealthReport,
  ModInfo,
  RestoreResult,
} from '../types';
import { mock } from './mock';
import { callRust, NATIVE } from './tauri';

export async function scanConflicts(stagingDir: string): Promise<ConflictReport> {
  if (!NATIVE) return mock.scanConflicts();
  return callRust<ConflictReport>('scan_conflicts', { stagingDir });
}

export async function deployMods(
  gameDir: string,
  stagingDir: string
): Promise<DeployResult> {
  if (!NATIVE) return mock.deployMods(gameDir);
  return callRust<DeployResult>('deploy_mods', { gameDir, stagingDir });
}

export async function restoreMods(gameDir: string): Promise<RestoreResult> {
  if (!NATIVE) return mock.restoreMods(gameDir);
  return callRust<RestoreResult>('restore_mods', { gameDir });
}

export async function diagnoseEnv(
  gameDir: string,
  stagingDir: string
): Promise<HealthReport> {
  if (!NATIVE) return mock.diagnoseEnv(gameDir, stagingDir);
  return callRust<HealthReport>('diagnose_env', { gameDir, stagingDir });
}

export async function setupModEngine(
  gameDir: string,
  stagingDir: string
): Promise<void> {
  if (!NATIVE) return;
  return callRust<void>('setup_mod_engine', { gameDir, stagingDir });
}

export async function provisionEngineMod(stagingDir: string): Promise<ModInfo> {
  if (!NATIVE) return mock.provisionEngineMod();
  return callRust<ModInfo>('provision_engine_mod', { stagingDir });
}

export async function launchGame(gameDir: string): Promise<void> {
  const trimmed = gameDir.trim();
  if (!trimmed) {
    throw new Error('请先在设置中指定只狼游戏安装目录');
  }
  if (!NATIVE) {
    throw new Error('启动游戏仅在桌面客户端环境下可用');
  }
  await callRust<void>('launch_game', { gameDir: trimmed });
}