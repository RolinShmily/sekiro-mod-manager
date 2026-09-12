import React, { useState, useEffect, useCallback, useMemo } from 'react';
import { HeaderBar } from './components/HeaderBar';
import { ModList } from './components/ModList';
import { ModDetails } from './components/ModDetails';
import { DoctorModal } from './components/DoctorModal';
import { ImportModal } from './components/ImportModal';
import { SettingsModal } from './components/SettingsModal';
import { ConfirmModal } from './components/ConfirmModal';
import { ExportModModal } from './components/ExportModModal';
import { ExportModpackModal } from './components/ExportModpackModal';
import { ImportModpackModal } from './components/ImportModpackModal';
import { Toast } from './components/Toast';
import {
  getSettings,
  saveSettings,
  listMods,
  getModDetails,
  toggleMod,
  setModPriority,
  deleteMod,
  importModFile,
  scanConflicts,
  deployMods,
  restoreMods,
  diagnoseEnv,
  setupModEngine,
  provisionEngineMod,
} from './api';
import type {
  AppSettings,
  ConflictReport,
  HealthReport,
  ModDetailsPayload,
  ModInfo,
  ModPackManifest,
  ModSummary,
  ToastMessage,
} from './types';
import { formatBytes } from './utils/format';

export const App: React.FC = () => {
  // State
  const [settings, setSettings] = useState<AppSettings>({
    game_dir: '',
    staging_dir: '',
  });

  const [mods, setMods] = useState<ModSummary[]>([]);
  const [selectedModId, setSelectedModId] = useState<string | null>(null);
  const [modDetails, setModDetails] = useState<ModDetailsPayload | null>(null);
  const [conflicts, setConflicts] = useState<ConflictReport | null>(null);
  const [health, setHealth] = useState<HealthReport | null>(null);

  // Loading & Action States
  const [isLoadingMods, setIsLoadingMods] = useState(false);
  const [isLoadingDetails, setIsLoadingDetails] = useState(false);
  const [isDeploying, setIsDeploying] = useState(false);
  const [isRestoring, setIsRestoring] = useState(false);
  const [isFixingEngine, setIsFixingEngine] = useState(false);
  const [isImporting, setIsImporting] = useState(false);

  // Modals
  const [isDoctorOpen, setIsDoctorOpen] = useState(false);
  const [isRestoreModalOpen, setIsRestoreModalOpen] = useState(false);
  const [isImportOpen, setIsImportOpen] = useState(false);
  const [isSettingsOpen, setIsSettingsOpen] = useState(false);
  const [isExportModOpen, setIsExportModOpen] = useState(false);
  const [modToExport, setModToExport] = useState<ModInfo | null>(null);
  const [isExportModpackOpen, setIsExportModpackOpen] = useState(false);
  const [isImportModpackOpen, setIsImportModpackOpen] = useState(false);
  const [initialModpackPath, setInitialModpackPath] = useState('');
  const [initialImportSourcePath, setInitialImportSourcePath] = useState('');

  // Toasts
  const [toasts, setToasts] = useState<ToastMessage[]>([]);

  const dismissToast = useCallback((id: string) => {
    setToasts((prev) => prev.filter((t) => t.id !== id));
  }, []);

  const showToast = useCallback(
    (toast: Omit<ToastMessage, 'id'>) => {
      const id = Math.random().toString(36).substring(2, 9);
      const newToast: ToastMessage = { ...toast, id };
      setToasts((prev) => [...prev, newToast]);

      const duration = toast.duration || (toast.type === 'error' ? 6000 : 4500);
      setTimeout(() => {
        dismissToast(id);
      }, duration);
    },
    [dismissToast]
  );

  // Compute set of colliding mod IDs
  const conflictModIds = useMemo(() => {
    const set = new Set<string>();
    if (!conflicts || !conflicts.records) return set;
    for (const r of conflicts.records) {
      set.add(r.winner_mod_id);
      for (const sid of r.shadowed_mod_ids) {
        set.add(sid);
      }
    }
    return set;
  }, [conflicts]);

  const selectMod = useCallback(
    async (modId: string, currentStagingDir?: string) => {
      const staging = currentStagingDir || settings.staging_dir;
      setSelectedModId(modId);
      if (!staging) return;

      setIsLoadingDetails(true);
      try {
        const details = await getModDetails(staging, modId);
        setModDetails(details);
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '获取模组详情失败',
          message: String(err),
        });
      } finally {
        setIsLoadingDetails(false);
      }
    },
    [settings.staging_dir, showToast]
  );

  const refreshMods = useCallback(
    async (currentStagingDir?: string) => {
      const staging = currentStagingDir || settings.staging_dir;
      if (!staging) return;

      setIsLoadingMods(true);
      try {
        const list = await listMods(staging);
        setMods(list);

        if (list.length > 0) {
          setSelectedModId((prevSelected) => {
            const exists = prevSelected && list.some((m) => m.id === prevSelected);
            const targetId = exists ? prevSelected : list[0].id;
            // Trigger selection
            selectMod(targetId, staging);
            return targetId;
          });
        } else {
          setSelectedModId(null);
          setModDetails(null);
        }
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '读取模组列表失败',
          message: String(err),
        });
      } finally {
        setIsLoadingMods(false);
      }
    },
    [settings.staging_dir, selectMod, showToast]
  );

  const refreshConflicts = useCallback(
    async (currentStagingDir?: string) => {
      const staging = currentStagingDir || settings.staging_dir;
      if (!staging) return;
      try {
        const report = await scanConflicts(staging);
        setConflicts(report);
      } catch (err: any) {
        console.error('Scan conflicts error:', err);
      }
    },
    [settings.staging_dir]
  );

  const refreshHealth = useCallback(
    async (currentGameDir?: string, currentStagingDir?: string) => {
      const game = currentGameDir || settings.game_dir;
      const staging = currentStagingDir || settings.staging_dir;
      if (!game) {
        setHealth(null);
        return;
      }
      try {
        const report = await diagnoseEnv(game, staging);
        setHealth(report);
      } catch (err: any) {
        console.error('Diagnose error:', err);
      }
    },
    [settings.game_dir, settings.staging_dir]
  );

  const refreshAll = useCallback(
    async (s?: AppSettings) => {
      const curSettings = s || settings;
      await Promise.all([
        refreshMods(curSettings.staging_dir),
        refreshConflicts(curSettings.staging_dir),
        refreshHealth(curSettings.game_dir, curSettings.staging_dir),
      ]);
    },
    [settings, refreshMods, refreshConflicts, refreshHealth]
  );

  // Initialize
  useEffect(() => {
    async function init() {
      try {
        const s = await getSettings();
        setSettings(s);
        await refreshAll(s);
        if (s.active_modal === 'export_modpack') {
          setIsExportModpackOpen(true);
        } else if (s.active_modal === 'export_mod') {
          setIsExportModOpen(true);
        } else if (s.active_modal === 'import_modpack') {
          setInitialModpackPath('exports/sekiro_pack_v1.0.0.smmpack');
          setIsImportModpackOpen(true);
        } else if (s.active_modal === 'import') {
          setIsImportOpen(true);
        } else if (s.active_modal === 'import_smmpack') {
          setInitialImportSourcePath('exports/sekiro_pack_v1.0.0.smmpack');
          setIsImportOpen(true);
        }
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '初始化失败',
          message: String(err),
        });
      }
    }
    init();
  }, []);

  const handleToggleMod = useCallback(
    async (modId: string, enabled: boolean) => {
      try {
        await toggleMod(settings.staging_dir, modId, enabled);
        setMods((prev) =>
          prev.map((m) => (m.id === modId ? { ...m, enabled } : m))
        );
        setModDetails((prev) => {
          if (prev && prev.info.id === modId) {
            return {
              ...prev,
              info: { ...prev.info, enabled },
            };
          }
          return prev;
        });
        await refreshConflicts();
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '启停操作失败',
          message: String(err),
        });
      }
    },
    [settings.staging_dir, refreshConflicts, showToast]
  );

  const handleMoveUp = useCallback(
    async (modId: string) => {
      const idx = mods.findIndex((m) => m.id === modId);
      if (idx <= 0) return;

      const targetMod = mods[idx - 1];
      const targetPri = targetMod.priority;
      const newPri = targetPri > 1 ? targetPri - 1 : 1;

      try {
        await setModPriority(settings.staging_dir, modId, newPri);
        await refreshMods();
        await refreshConflicts();
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '调整优先级失败',
          message: String(err),
        });
      }
    },
    [mods, settings.staging_dir, refreshMods, refreshConflicts, showToast]
  );

  const handleMoveDown = useCallback(
    async (modId: string) => {
      const idx = mods.findIndex((m) => m.id === modId);
      if (idx < 0 || idx >= mods.length - 1) return;

      const targetMod = mods[idx + 1];
      const newPri = targetMod.priority + 5;

      try {
        await setModPriority(settings.staging_dir, modId, newPri);
        await refreshMods();
        await refreshConflicts();
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '调整优先级失败',
          message: String(err),
        });
      }
    },
    [mods, settings.staging_dir, refreshMods, refreshConflicts, showToast]
  );

  const handleDeleteMod = useCallback(
    async (modId: string) => {
      if (!window.confirm(`确认永久删除模组 '${modId}' 吗？此操作无法撤销。`)) {
        return;
      }
      try {
        await deleteMod(settings.staging_dir, modId);
        showToast({
          type: 'info',
          title: '模组已删除',
          message: `已从暂存库彻底删除 '${modId}'。`,
        });
        await refreshAll();
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '删除模组失败',
          message: String(err),
        });
      }
    },
    [settings.staging_dir, showToast, refreshAll]
  );

  // Core Deployment Action
  const handleDeploy = useCallback(async () => {
    if (!settings.game_dir || !settings.staging_dir) {
      showToast({
        type: 'warning',
        title: '路径未就绪',
        message: '请先在设置中指定有效的游戏路径与暂存库路径。',
      });
      return;
    }

    setIsDeploying(true);
    try {
      const res = await deployMods(settings.game_dir, settings.staging_dir);
      const savedStr =
        res.bytes_saved > 0 ? formatBytes(res.bytes_saved) : '100% 磁盘空间';

      showToast({
        type: 'success',
        title: '一键秒级部署完成！',
        message: `已为游戏生成 ${res.hard_links_created} 个 NTFS 硬链接映射，耗时仅 ${res.duration_ms}ms。`,
        duration: 7000,
        stats: {
          links: res.hard_links_created,
          durationMs: Number(res.duration_ms),
          savedBytes: savedStr,
        },
      });

      await refreshHealth();
    } catch (err: any) {
      showToast({
        type: 'error',
        title: '部署失败',
        message: String(err),
      });
    } finally {
      setIsDeploying(false);
    }
  }, [settings.game_dir, settings.staging_dir, showToast, refreshHealth]);

  // Restore Action
  const handleRestoreClick = useCallback(() => {
    setIsRestoreModalOpen(true);
  }, []);

  const handleConfirmRestore = useCallback(async () => {
    setIsRestoreModalOpen(false);
    setIsRestoring(true);
    try {
      const res = await restoreMods(settings.game_dir);
      showToast({
        type: 'info',
        title: '还原纯净完成',
        message: `已清理 ${res.removed_files} 个部署文件与链接，耗时 ${res.duration_ms}ms。`,
      });
      await refreshHealth();
    } catch (err: any) {
      showToast({
        type: 'error',
        title: '还原失败',
        message: String(err),
      });
    } finally {
      setIsRestoring(false);
    }
  }, [settings.game_dir, showToast, refreshHealth]);

  // Fix ModEngine Action
  const handleFixModEngine = useCallback(async () => {
    setIsFixingEngine(true);
    try {
      await setupModEngine(settings.game_dir, settings.staging_dir);
      // Also provision to staging so it appears in mod list
      try {
        await provisionEngineMod(settings.staging_dir);
        await refreshMods();
      } catch (_) {}
      showToast({
        type: 'success',
        title: 'ModEngine 装配成功',
        message: '已自动部署 dinput8.dll 并生成/修复标准 modengine.ini 配置，已同步至模组列表。',
      });
      await refreshHealth();
    } catch (err: any) {
      showToast({
        type: 'error',
        title: '装配 ModEngine 失败',
        message: String(err),
      });
    } finally {
      setIsFixingEngine(false);
    }
  }, [settings.game_dir, settings.staging_dir, showToast, refreshMods, refreshHealth]);

  // Provision ModEngine to staging mod list
  const handleProvisionModEngine = useCallback(async () => {
    setIsFixingEngine(true);
    try {
      const info = await provisionEngineMod(settings.staging_dir);
      showToast({
        type: 'success',
        title: '已成功装配 Sekiro Mod Engine',
        message: `模组 [${info.name}] 已加入暂存库，包含核心挂钩驱动 dinput8.dll 与标准 modengine.ini 配置。`,
      });
      await refreshMods();
      await refreshConflicts();
      await refreshHealth();
    } catch (err: any) {
      showToast({
        type: 'error',
        title: '装配 ModEngine 失败',
        message: String(err),
      });
    } finally {
      setIsFixingEngine(false);
    }
  }, [settings.staging_dir, showToast, refreshMods, refreshConflicts, refreshHealth]);

  // Import Action
  const handleImportMod = useCallback(
    async (sourcePath: string, sourceUrl?: string) => {
      setIsImporting(true);
      try {
        const imported = await importModFile(sourcePath, settings.staging_dir, sourceUrl);
        setIsImportOpen(false);
        showToast({
          type: 'success',
          title: '模组导入成功！',
          message: `成功归一化并导入模组 '${imported.name}' (${imported.id})。`,
        });
        await refreshAll();
        await selectMod(imported.id);
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '导入模组失败',
          message: String(err),
        });
      } finally {
        setIsImporting(false);
      }
    },
    [settings.staging_dir, showToast, refreshAll, selectMod]
  );

  // Modpack & Single Mod Handlers
  const handleOpenExportMod = useCallback((mod: ModInfo) => {
    setModToExport(mod);
    setIsExportModOpen(true);
  }, []);

  const handleExportSingleModSuccess = useCallback(
    (outputPath: string) => {
      showToast({
        type: 'success',
        title: '单模组导出成功！',
        message: `已打包输出至: ${outputPath}`,
      });
    },
    [showToast]
  );

  const handleOpenExportModpack = useCallback(() => {
    setIsExportModpackOpen(true);
  }, []);

  const handleExportModpackSuccess = useCallback(
    (outputPath: string) => {
      setIsExportModpackOpen(false);
      showToast({
        type: 'success',
        title: '整合包打包完成！',
        message: `成功导出 .smmpack 容器至: ${outputPath}`,
        duration: 6000,
      });
    },
    [showToast]
  );

  const handleOpenImportModpack = useCallback((packPath?: string) => {
    setInitialModpackPath(packPath || '');
    setIsImportModpackOpen(true);
  }, []);

  const handleRouteToModpackImport = useCallback((packPath: string) => {
    setIsImportOpen(false);
    setInitialModpackPath(packPath);
    setIsImportModpackOpen(true);
  }, []);

  const handleImportModpackSuccess = useCallback(
    async (manifest: ModPackManifest) => {
      showToast({
        type: 'success',
        title: '整合包导入纳管成功！',
        message: `已批量解包「${manifest.name}」v${manifest.version}，共纳管 ${manifest.mods.length} 款模组。`,
        duration: 6000,
      });
      await refreshAll();
    },
    [showToast, refreshAll]
  );

  const handleModUpdated = useCallback(
    (updated: ModInfo) => {
      setMods((prev) =>
        prev.map((m) =>
          m.id === updated.id
            ? {
                ...m,
                name: updated.name,
                version: updated.version,
                author: updated.author,
                category: updated.category,
                source_url: updated.source_url,
                homepage: updated.homepage,
                description: updated.description,
              }
            : m
        )
      );
      setModDetails((prev) => {
        if (prev && prev.info.id === updated.id) {
          return {
            ...prev,
            info: updated,
          };
        }
        return prev;
      });
      refreshConflicts();
    },
    [refreshConflicts]
  );

  // Global QA & Accessibility Hotkeys
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'F6') {
        const targetMod = modDetails?.info || (mods.length > 0 ? mods[0] : null);
        if (targetMod) {
          handleOpenExportMod(targetMod);
        }
      } else if (e.key === 'F7') {
        handleOpenExportModpack();
      } else if (e.key === 'F8') {
        handleOpenImportModpack('exports/sekiro_pack_v1.0.0.smmpack');
      } else if (e.key === 'F9') {
        setIsImportOpen(true);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [mods, modDetails, handleOpenExportMod, handleOpenExportModpack, handleOpenImportModpack]);

  // Save Settings Action
  const handleSaveSettings = useCallback(
    async (newSettings: AppSettings) => {
      try {
        const saved = await saveSettings(newSettings);
        setSettings(saved);
        setIsSettingsOpen(false);
        showToast({
          type: 'success',
          title: '配置已更新',
          message: '游戏与暂存库路径已保存，正在重新扫描...',
        });
        await refreshAll(saved);
      } catch (err: any) {
        showToast({
          type: 'error',
          title: '保存配置失败',
          message: String(err),
        });
      }
    },
    [showToast, refreshAll]
  );

  return (
    <div className="flex flex-col h-screen w-screen overflow-hidden bg-canvas text-ink font-sans antialiased">
      {/* Top Persistent HeaderBar */}
      <HeaderBar
        settings={settings}
        health={health}
        conflicts={conflicts}
        isDeploying={isDeploying}
        isRestoring={isRestoring}
        onOpenSettings={() => setIsSettingsOpen(true)}
        onOpenDoctor={() => setIsDoctorOpen(true)}
        onOpenConflicts={() => {
          // If there are conflicts, ensure a conflicting mod is selected
          if (conflicts && conflicts.records.length > 0) {
            const firstCollidingMod = conflicts.records[0].winner_mod_id;
            if (firstCollidingMod) {
              selectMod(firstCollidingMod);
            }
          }
        }}
        onOpenImportModpack={() => handleOpenImportModpack()}
        onOpenExportModpack={handleOpenExportModpack}
        onDeploy={handleDeploy}
        onRestore={handleRestoreClick}
      />

      {/* Main Dual Column Workspace */}
      <div className="flex flex-1 overflow-hidden">
        {/* Left Column: Mod List */}
        <ModList
          mods={mods}
          selectedModId={selectedModId}
          conflictModIds={conflictModIds}
          isLoading={isLoadingMods}
          isProvisioningEngine={isFixingEngine}
          onSelect={selectMod}
          onToggle={handleToggleMod}
          onMoveUp={handleMoveUp}
          onMoveDown={handleMoveDown}
          onDelete={handleDeleteMod}
          onOpenImport={() => setIsImportOpen(true)}
          onOpenImportModpack={() => handleOpenImportModpack()}
          onOpenExportModpack={handleOpenExportModpack}
          onProvisionModEngine={handleProvisionModEngine}
        />

        {/* Right Column: PDP Product Details & Inspection */}
        <ModDetails
          stagingDir={settings.staging_dir}
          details={modDetails}
          conflicts={conflicts}
          isLoading={isLoadingDetails}
          initialEditOpen={settings.active_modal === 'edit_mod'}
          onUpdateModInfo={handleModUpdated}
          onOpenExportMod={handleOpenExportMod}
          onShowToast={showToast}
        />
      </div>

      {/* Modals */}
      <DoctorModal
        isOpen={isDoctorOpen}
        health={health}
        isFixing={isFixingEngine}
        onClose={() => setIsDoctorOpen(false)}
        onFixModEngine={handleFixModEngine}
      />

      <ImportModal
        isOpen={isImportOpen}
        stagingDir={settings.staging_dir}
        isImporting={isImporting}
        initialSourcePath={initialImportSourcePath}
        onClose={() => setIsImportOpen(false)}
        onImport={handleImportMod}
        onRouteToModpackImport={handleRouteToModpackImport}
      />

      <ExportModModal
        isOpen={isExportModOpen}
        stagingDir={settings.staging_dir}
        mod={modToExport || modDetails?.info || (mods.length > 0 ? mods[0] : null)}
        onClose={() => setIsExportModOpen(false)}
        onExportSuccess={handleExportSingleModSuccess}
      />

      <ExportModpackModal
        isOpen={isExportModpackOpen}
        stagingDir={settings.staging_dir}
        mods={mods}
        onClose={() => setIsExportModpackOpen(false)}
        onExportSuccess={handleExportModpackSuccess}
      />

      <ImportModpackModal
        isOpen={isImportModpackOpen}
        stagingDir={settings.staging_dir}
        initialPackPath={initialModpackPath}
        onClose={() => setIsImportModpackOpen(false)}
        onImportSuccess={handleImportModpackSuccess}
      />

      <SettingsModal
        isOpen={isSettingsOpen}
        settings={settings}
        onClose={() => setIsSettingsOpen(false)}
        onSave={handleSaveSettings}
      />

      <ConfirmModal
        isOpen={isRestoreModalOpen}
        title="还原纯净环境确认"
        subtitle="VANILLA ROLLBACK CONFIRMATION"
        message="确认清空游戏 mods 目录中的所有部署链接吗？游戏将恢复为纯净原生状态。"
        detail="本操作将安全卸载由只狼模组管理器挂载到游戏目录中的所有硬链接与核心加载器（dinput8.dll / modengine.ini）。"
        confirmText="确认还原纯净"
        cancelText="取消"
        isConfirming={isRestoring}
        onConfirm={handleConfirmRestore}
        onCancel={() => setIsRestoreModalOpen(false)}
      />

      {/* Toast Notifications */}
      <Toast toasts={toasts} onDismiss={dismissToast} />
    </div>
  );
};

export default App;
