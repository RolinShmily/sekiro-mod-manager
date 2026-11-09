#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include "backend/GuiTypes.hpp"
#include "backend/services/TaskTypes.hpp"

#include "smm/doctor.hpp"
#include "smm/loader.hpp"

/// Synchronous application service in front of `smm_core`.
///
/// Responsibilities, and only these:
///   * translate GUI request shapes (QString ids, paths) into core calls;
///   * run the small orchestration that spans several core calls for one user action
///     (scan + conflicts + plan; import + preset; rename + metadata write);
///   * translate core results into the Qt projections in GuiTypes.hpp.
///
/// Deliberately absent: threads, Qt signals, busy/UI state, `tr()` strings, cancellation
/// policy and QSettings. Those belong to TaskRunner and GuiController. Every method is
/// static and thread-safe because it holds no state; failures surface as `smm::SmmError`
/// (stable `ErrorCode`) or `std::runtime_error`, so the controller decides how to phrase and
/// where to deliver them.
namespace smm::gui {

class CoreService {
public:
    /// One scan of the staging directory, in both projections.
    struct Snapshot {
        StagingSnapshot view;
        smm::ScanOutcome scanned;
    };

    // --- reads -----------------------------------------------------------------
    static Snapshot scan(const QString& stagingDir);
    static EnvironmentInfo environment(const QString& stagingDir, const QString& gameDir);
    static ModEntry modDetail(const QString& stagingDir, const QString& modId,
                              QVector<AssetEntry>* assets);
    static QVector<PresetEntry> presets(const QString& stagingDir);
    /// Mod ids a preset enables, in its stored priority order. Empty when it is gone.
    static QStringList presetModIds(const QString& stagingDir, const QString& presetId);

    /// Health report for the environment. `snapshot` reuses an earlier staging scan instead
    /// of walking the tree again; pass nullptr to scan inside core.
    static HealthInfo diagnose(const QString& gameDir, const QString& stagingDir,
                               const Snapshot* snapshot);
    /// True when the process runs with an administrator token.
    static bool isElevated();

    /// Deployed file count and linked bytes, read from the manifest core owns.
    struct DeploymentTelemetry {
        int files{0};
        quint64 bytes{0};
    };
    static DeploymentTelemetry deploymentTelemetry(const QString& gameDir);

    // --- mod mutations ---------------------------------------------------------
    static void setModEnabled(const QString& stagingDir, const QString& modId, bool enabled);
    /// Returns how many mods were actually updated.
    static int setModsEnabled(const QString& stagingDir, const QStringList& modIds, bool enabled);
    /// Returns how many mods were actually deleted.
    static int removeMods(const QString& stagingDir, const QStringList& modIds);
    static void setAssetEnabled(const QString& stagingDir, const QString& modId,
                                const QString& relPath, bool enabled);
    static void setModPriority(const QString& stagingDir, const QString& modId, quint32 priority);
    static void swapModPriorities(const QString& stagingDir, const QString& modIdA, quint32 priA,
                                  const QString& modIdB, quint32 priB);
    static void batchUpdatePriorities(const QString& stagingDir,
                                      const QMap<QString, quint32>& priorities);
    /// Persists a drag-to-reorder: entry i receives priority (i + 1) * 10.
    static void reorderMods(const QString& stagingDir, const QStringList& orderedIds);
    static void setModPreview(const QString& stagingDir, const QString& modId,
                              const QString& imagePath);

    /// Applies user-edited metadata and, when `newId` differs, renames the staged mod.
    /// Returns the id the mod ends up with.
    static QString editModMetadata(const QString& stagingDir, const QString& modId,
                                   const QString& newId, const QString& name, const QString& author,
                                   const QString& version, const QString& category,
                                   const QString& description, const QString& sourceUrl);

    // --- presets ---------------------------------------------------------------
    static void savePreset(const QString& stagingDir, const QString& name,
                           const QString& description, const QString& nameEn,
                           const QString& descriptionEn);
    /// Returns the preset name that was applied.
    static QString applyPreset(const QString& stagingDir, const QString& presetId);
    static void deletePreset(const QString& stagingDir, const QString& presetId);

    // --- import / export -------------------------------------------------------
    /// Imports every path in order, reporting each result through `onImported` (GUI thread
    /// responsibility of the caller). Throws on the first failure, like the CLI does.
    static void importPaths(const QString& stagingDir, const QStringList& paths,
                            const QString& customId, const QString& customName, bool overwrite,
                            const ProgressSink& onProgress, const CancelToken& cancelled,
                            const std::function<void(const ImportSummary&)>& onImported);
    static ModPackImportSummary importModPack(const QString& stagingDir, const QString& packPath,
                                              const CancelToken& cancelled);
    static fs::path exportSingleMod(const QString& stagingDir, const QString& modId,
                                    const QString& outputPath);
    static fs::path exportModPack(const QString& stagingDir, const QStringList& modIds,
                                  const QString& outputPath, const QString& name,
                                  const QString& description, const ProgressSink& onProgress,
                                  const CancelToken& cancelled);

    // --- deployment ------------------------------------------------------------
    static DeploySummary deploy(const QString& stagingDir, const QString& gameDir,
                                const ProgressSink& onProgress, const CancelToken& cancelled);
    static RestoreSummary restore(const QString& gameDir, const ProgressSink& onProgress,
                                  const CancelToken& cancelled);
    struct EngineSetup {
        QString hookPath;
        QString iniPath;
        bool installed{false};
    };
    static EngineSetup setupEngine(const QString& gameDir, const QString& stagingDir);

    // --- maintenance -----------------------------------------------------------
    struct PreviewOptimization {
        int optimized{0};
        quint64 bytesSaved{0};
    };
    static PreviewOptimization optimizePreviews(const QString& stagingDir);

    /// Creates the staging directory when it is missing (a first run has none yet).
    static void ensureStagingDir(const QString& stagingDir);
};

} // namespace smm::gui
