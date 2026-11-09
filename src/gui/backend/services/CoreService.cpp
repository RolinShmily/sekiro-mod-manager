#include "backend/services/CoreService.hpp"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <fstream>
#include <system_error>

#include "backend/adapters/CoreMapping.hpp"

#include "smm/conflict.hpp"
#include "smm/deploy.hpp"
#include "smm/error.hpp"
#include "smm/executor.hpp"
#include "smm/exporter.hpp"
#include "smm/extractor.hpp"
#include "smm/importer.hpp"
#include "smm/manager.hpp"
#include "smm/preset.hpp"

namespace smm::gui {

namespace {

/// Core progress callback that also enforces cancellation.
///
/// Throwing SmmError(Cancelled) from inside the progress callback is the cooperative stop:
/// core calls it between files, so no file is left half-written.
smm::DeployProgressCallback deployProgress(const ProgressSink& onProgress,
                                           const CancelToken& cancelled) {
    return [onProgress, cancelled](const smm::DeployProgress& progress) {
        if (cancelled && cancelled()) {
            smm::fail(smm::ErrorCode::Cancelled, "Cancelled by the user");
        }
        if (onProgress) {
            onProgress(static_cast<int>(progress.done), static_cast<int>(progress.total),
                       QString::fromStdString(progress.current_path));
        }
    };
}

smm::ExtractProgressCallback extractProgress(const ProgressSink& onProgress,
                                             const CancelToken& cancelled) {
    return [onProgress, cancelled](std::size_t done, std::size_t total) {
        if (cancelled && cancelled()) {
            smm::fail(smm::ErrorCode::Cancelled, "Cancelled by the user");
        }
        if (onProgress) {
            onProgress(static_cast<int>(done), static_cast<int>(total), QString{});
        }
    };
}

smm::ExportProgressCallback exportProgress(const ProgressSink& onProgress,
                                           const CancelToken& cancelled) {
    return [onProgress, cancelled](std::size_t done, std::size_t total,
                                   const std::string& current) {
        if (cancelled && cancelled()) {
            smm::fail(smm::ErrorCode::Cancelled, "Cancelled by the user");
        }
        if (onProgress) {
            onProgress(static_cast<int>(done), static_cast<int>(total),
                       QString::fromStdString(current));
        }
    };
}

bool stagingExists(const fs::path& staging) {
    std::error_code ec;
    return !staging.empty() && fs::is_directory(staging, ec);
}

bool pathExists(const fs::path& path) {
    std::error_code ec;
    return !path.empty() && fs::exists(path, ec);
}

} // namespace

// ---------------------------------------------------------------------------
// Reads
// ---------------------------------------------------------------------------

CoreService::Snapshot CoreService::scan(const QString& stagingDir) {
    Snapshot snapshot;
    const fs::path staging = toStdPath(stagingDir);
    if (!stagingExists(staging)) {
        return snapshot;
    }

    snapshot.scanned = smm::ModLoader::scan_mods_directory(staging);
    const auto& mods = snapshot.scanned.mods;
    const auto report = smm::ConflictEngine::scan_conflicts(mods);
    const auto plan = smm::DeploymentPlanner::build_plan("default", mods);

    auto& view = snapshot.view;
    view.scanned = true;
    view.mods.reserve(static_cast<qsizetype>(mods.size()));
    for (const auto& mod : mods) {
        view.mods.append(toModEntry(mod));
    }
    view.failures.reserve(static_cast<qsizetype>(snapshot.scanned.failures.size()));
    for (const auto& failure : snapshot.scanned.failures) {
        view.failures.append(toQString(failure.path) + QStringLiteral(": ") +
                             QString::fromStdString(failure.message));
    }
    view.conflicts.reserve(static_cast<qsizetype>(report.records.size()));
    for (const auto& record : report.records) {
        view.conflicts.append(toConflictEntry(record));
    }
    view.plan = toPlanInfo(plan);
    view.plan.conflicts = view.conflicts;
    return snapshot;
}

EnvironmentInfo CoreService::environment(const QString& stagingDir, const QString& gameDir) {
    EnvironmentInfo info;
    info.settingsFile = QStringLiteral("QSettings:HKCU/Software/SekiroModManager/SMM");

    fs::path staging = toStdPath(stagingDir);
    if (staging.empty()) {
        staging = smm::detect_staging_dir();
    }
    if (!staging.empty()) {
        std::error_code ec;
        fs::create_directories(staging, ec);
    }
    info.stagingDir = toQString(staging);
    info.stagingExists = stagingExists(staging);

    fs::path game = toStdPath(gameDir);
    if (game.empty()) {
        game = smm::detect_game_dir();
    }
    info.gameDir = toQString(game);
    info.gameDirFound = !game.empty();
    info.gameDirExists = pathExists(game);
    return info;
}

ModEntry CoreService::modDetail(const QString& stagingDir, const QString& modId,
                                QVector<AssetEntry>* assets) {
    const smm::StagedMod mod =
        smm::ModManager::get_mod_details(toStdPath(stagingDir), modId.toStdString());
    if (assets != nullptr) {
        assets->clear();
        assets->reserve(static_cast<qsizetype>(mod.assets.size()));
        for (const auto& asset : mod.assets) {
            assets->append(toAssetEntry(asset));
        }
    }
    return toModEntry(mod);
}

QVector<PresetEntry> CoreService::presets(const QString& stagingDir) {
    QVector<PresetEntry> entries;
    const fs::path staging = toStdPath(stagingDir);
    if (!stagingExists(staging)) {
        return entries;
    }
    for (const auto& preset : smm::PresetManager::list_presets(staging)) {
        PresetEntry entry;
        entry.id = QString::fromStdString(preset.id);
        entry.name = QString::fromStdString(preset.name);
        entry.description =
            preset.description ? QString::fromStdString(*preset.description) : QString{};
        entry.nameEn = preset.name_en ? QString::fromStdString(*preset.name_en) : QString{};
        entry.descriptionEn =
            preset.description_en ? QString::fromStdString(*preset.description_en) : QString{};
        entry.modCount = static_cast<int>(preset.mods.size());
        entry.updatedAt = preset.updated_at;
        entries.append(entry);
    }
    return entries;
}

QStringList CoreService::presetModIds(const QString& stagingDir, const QString& presetId) {
    QStringList ids;
    const fs::path staging = toStdPath(stagingDir);
    if (presetId.isEmpty() || !stagingExists(staging)) {
        return ids;
    }
    for (const auto& preset : smm::PresetManager::list_presets(staging)) {
        if (QString::fromStdString(preset.id) != presetId) {
            continue;
        }
        ids.reserve(static_cast<qsizetype>(preset.mods.size()));
        for (const auto& entry : preset.mods) {
            ids.append(QString::fromStdString(entry.mod_id));
        }
        break;
    }
    return ids;
}

HealthInfo CoreService::diagnose(const QString& gameDir, const QString& stagingDir,
                                 const Snapshot* snapshot) {
    const smm::ScanOutcome* preloaded =
        snapshot != nullptr && snapshot->view.scanned ? &snapshot->scanned : nullptr;
    try {
        return toHealthInfo(smm::diagnose_environment(toStdPath(gameDir), toStdPath(stagingDir),
                                                      true, preloaded));
    } catch (const std::exception& error) {
        // Diagnostics report problems instead of throwing: a failure to inspect the
        // environment is itself a diagnostic finding the user has to act on.
        HealthInfo failed;
        failed.gameDir = gameDir;
        failed.stagingDir = stagingDir;
        failed.overall = QStringLiteral("action_required");
        failed.errorCount = 1;
        failed.items.append(DiagnosticEntry{QStringLiteral("doctor-failed"),
                                            QStringLiteral("diagnostic"),
                                            QStringLiteral("Diagnostics did not complete"),
                                            QString::fromUtf8(error.what()),
                                            QStringLiteral("Check directory access and retry."),
                                            QStringLiteral("error")});
        return failed;
    }
}

bool CoreService::isElevated() {
    return smm::is_process_elevated();
}

CoreService::DeploymentTelemetry CoreService::deploymentTelemetry(const QString& gameDir) {
    DeploymentTelemetry telemetry;
    if (gameDir.isEmpty()) {
        return telemetry;
    }
    const auto manifest = smm::load_deploy_manifest(smm::resolve_mods_dir(toStdPath(gameDir)));
    if (manifest.has_value()) {
        telemetry.files = static_cast<int>(manifest->files.size());
        telemetry.bytes = manifest->total_bytes;
    }
    return telemetry;
}

// ---------------------------------------------------------------------------
// Mod mutations
// ---------------------------------------------------------------------------

void CoreService::setModEnabled(const QString& stagingDir, const QString& modId, bool enabled) {
    smm::ModManager::set_mod_enabled(toStdPath(stagingDir), modId.toStdString(), enabled);
}

int CoreService::setModsEnabled(const QString& stagingDir, const QStringList& modIds,
                                bool enabled) {
    const fs::path staging = toStdPath(stagingDir);
    int updated = 0;
    for (const auto& id : smm::gui::toIdVector(modIds)) {
        try {
            smm::ModManager::set_mod_enabled(staging, id, enabled);
            ++updated;
        } catch (const smm::SmmError& error) {
            if (error.code() == smm::ErrorCode::ModNotFound) {
                continue;
            }
            throw;
        }
    }
    return updated;
}

int CoreService::removeMods(const QString& stagingDir, const QStringList& modIds) {
    const fs::path staging = toStdPath(stagingDir);
    int removed = 0;
    for (const auto& id : smm::gui::toIdVector(modIds)) {
        try {
            smm::ModManager::delete_mod(staging, id);
            ++removed;
        } catch (const smm::SmmError& error) {
            if (error.code() == smm::ErrorCode::ModNotFound) {
                continue;
            }
            throw;
        }
    }
    return removed;
}

void CoreService::setAssetEnabled(const QString& stagingDir, const QString& modId,
                                  const QString& relPath, bool enabled) {
    smm::ModManager::set_asset_enabled(toStdPath(stagingDir), modId.toStdString(),
                                       relPath.toStdString(), enabled);
}

void CoreService::setModPriority(const QString& stagingDir, const QString& modId,
                                 quint32 priority) {
    smm::ModManager::set_mod_priority(toStdPath(stagingDir), modId.toStdString(), priority);
}

void CoreService::swapModPriorities(const QString& stagingDir, const QString& modIdA, quint32 priA,
                                    const QString& modIdB, quint32 priB) {
    const fs::path staging = toStdPath(stagingDir);
    smm::ModManager::set_mod_priority(staging, modIdA.toStdString(), priA);
    smm::ModManager::set_mod_priority(staging, modIdB.toStdString(), priB);
}

void CoreService::batchUpdatePriorities(const QString& stagingDir,
                                        const QMap<QString, quint32>& priorities) {
    const fs::path staging = toStdPath(stagingDir);
    for (auto it = priorities.constBegin(); it != priorities.constEnd(); ++it) {
        smm::ModManager::set_mod_priority(staging, it.key().toStdString(), it.value());
    }
}

void CoreService::reorderMods(const QString& stagingDir, const QStringList& orderedIds) {
    smm::ModManager::reindex_priorities(toStdPath(stagingDir), smm::gui::toIdVector(orderedIds));
}

void CoreService::setModPreview(const QString& stagingDir, const QString& modId,
                                const QString& imagePath) {
    smm::ModManager::set_mod_preview(toStdPath(stagingDir), modId.toStdString(),
                                     toStdPath(imagePath));
}

QString CoreService::editModMetadata(const QString& stagingDir, const QString& modId,
                                     const QString& newId, const QString& name,
                                     const QString& author, const QString& version,
                                     const QString& category, const QString& description,
                                     const QString& sourceUrl) {
    const fs::path staging = toStdPath(stagingDir);
    fs::path modDir = smm::ModManager::find_mod_dir(staging, modId.toStdString());
    smm::ModInfo info = smm::ModLoader::load_mod_info(modDir);

    if (!name.trimmed().isEmpty()) info.name = name.trimmed().toStdString();
    if (!author.trimmed().isEmpty()) info.author = author.trimmed().toStdString();
    if (!version.trimmed().isEmpty()) info.version = version.trimmed().toStdString();
    if (!category.trimmed().isEmpty()) info.category = category.trimmed().toStdString();
    info.description = description.toStdString();
    info.source_url = sourceUrl.toStdString();

    QString finalId = modId;
    const QString requested = newId.trimmed();
    if (!requested.isEmpty()) {
        const std::string sanitized = smm::ModManager::sanitize_mod_id(requested.toStdString());
        if (!sanitized.empty() && sanitized != modId.toStdString()) {
            // Rename first: it moves the directory and rewrites the metadata id, so the field
            // edits below land in the new location exactly once.
            const smm::ModInfo renamed =
                smm::ModManager::rename_mod(staging, modId.toStdString(), requested.toStdString());
            finalId = QString::fromStdString(renamed.id);
            modDir = renamed.root_path ? *renamed.root_path : modDir;
            info.id = renamed.id;
        }
    }

    smm::ModManager::save_mod_info(modDir, info);
    return finalId;
}

// ---------------------------------------------------------------------------
// Presets
// ---------------------------------------------------------------------------

void CoreService::savePreset(const QString& stagingDir, const QString& name,
                             const QString& description, const QString& nameEn,
                             const QString& descriptionEn) {
    fs::path staging = toStdPath(stagingDir);
    if (!stagingExists(staging)) {
        staging = smm::detect_staging_dir(staging);
        std::error_code ec;
        fs::create_directories(staging, ec);
    }
    auto preset = smm::PresetManager::create_preset_from_current(
        staging, name.toStdString(), description.toStdString());
    if (!nameEn.trimmed().isEmpty()) {
        preset.name_en = nameEn.trimmed().toStdString();
    }
    if (!descriptionEn.trimmed().isEmpty()) {
        preset.description_en = descriptionEn.trimmed().toStdString();
    }
    smm::PresetManager::save_preset(staging, preset);
}

QString CoreService::applyPreset(const QString& stagingDir, const QString& presetId) {
    const auto applied =
        smm::PresetManager::apply_preset(toStdPath(stagingDir), presetId.toStdString());
    return QString::fromStdString(applied.name);
}

void CoreService::deletePreset(const QString& stagingDir, const QString& presetId) {
    smm::PresetManager::delete_preset(toStdPath(stagingDir), presetId.toStdString());
}

// ---------------------------------------------------------------------------
// Import / export
// ---------------------------------------------------------------------------

void CoreService::importPaths(const QString& stagingDir, const QStringList& paths,
                              const QString& customId, const QString& customName, bool overwrite,
                              const ProgressSink& onProgress, const CancelToken& cancelled,
                              const std::function<void(const ImportSummary&)>& onImported) {
    const fs::path staging = toStdPath(stagingDir);
    smm::ImportOptions options;
    if (!customId.isEmpty()) {
        options.id = customId.toStdString();
    }
    if (!customName.isEmpty()) {
        options.name = customName.toStdString();
    }
    options.overwrite = overwrite;

    const int total = static_cast<int>(paths.size());
    int index = 0;
    for (const auto& path : paths) {
        if (cancelled && cancelled()) {
            smm::fail(smm::ErrorCode::Cancelled, "Cancelled by the user");
        }
        if (onProgress) {
            onProgress(index, total, path);
        }
        ++index;

        const auto result = smm::import_mod(toStdPath(path), staging, options);
        if (onImported) {
            ImportSummary summary;
            summary.id = QString::fromStdString(result.info.id);
            summary.name = QString::fromStdString(result.info.name);
            summary.version = QString::fromStdString(result.info.version);
            summary.category = QString::fromStdString(result.info.category);
            summary.priority = result.info.priority;
            summary.assetCount = static_cast<int>(result.asset_count);
            summary.ignoredCount = static_cast<int>(result.ignored_count);
            summary.totalBytes = result.total_bytes;
            summary.replacedExisting = result.replaced_existing;
            summary.dryRun = result.dry_run;
            summary.location = toQString(result.mod_dir);
            onImported(summary);
        }
    }
}

ModPackImportSummary CoreService::importModPack(const QString& stagingDir, const QString& packPath,
                                                const CancelToken& cancelled) {
    if (cancelled && cancelled()) {
        smm::fail(smm::ErrorCode::Cancelled, "Cancelled by the user");
    }
    const fs::path staging = toStdPath(stagingDir);
    const auto result = smm::import_modpack(toStdPath(packPath), staging, true);

    // A pack describes a curated loadout, so it is also recorded as a preset: the user can
    // return to that exact combination later without re-importing anything.
    smm::ModPreset preset;
    preset.name = result.manifest.name;
    if (result.manifest.description) {
        preset.description = *result.manifest.description;
    }
    preset.mods.reserve(result.manifest.mods.size());
    for (const auto& item : result.manifest.mods) {
        preset.mods.push_back(smm::ModPresetEntry{item.id, item.priority});
    }
    smm::PresetManager::save_preset(staging, preset);

    ModPackImportSummary summary;
    summary.name = QString::fromStdString(result.manifest.name);
    summary.modCount = static_cast<int>(result.mods.size());
    return summary;
}

fs::path CoreService::exportSingleMod(const QString& stagingDir, const QString& modId,
                                      const QString& outputPath) {
    const fs::path staging = toStdPath(stagingDir);
    fs::path output = toStdPath(outputPath);
    if (output.empty()) {
        const fs::path exportDir = staging / "exports";
        std::error_code ec;
        fs::create_directories(exportDir, ec);
        output = exportDir / (modId.toStdString() + ".zip");
    }
    return smm::export_single_mod(staging, modId.toStdString(), output);
}

fs::path CoreService::exportModPack(const QString& stagingDir, const QStringList& modIds,
                                    const QString& outputPath, const QString& name,
                                    const QString& description, const ProgressSink& onProgress,
                                    const CancelToken& cancelled) {
    const fs::path staging = toStdPath(stagingDir);
    const fs::path output = outputPath.isEmpty()
                                ? staging / (name.toStdString() + ".smmpack")
                                : toStdPath(outputPath);
    return smm::export_modpack(staging, smm::gui::toIdVector(modIds), output, name.toStdString(),
                               description.toStdString(), {},
                               exportProgress(onProgress, cancelled));
}

// ---------------------------------------------------------------------------
// Deployment
// ---------------------------------------------------------------------------

DeploySummary CoreService::deploy(const QString& stagingDir, const QString& gameDir,
                                  const ProgressSink& onProgress, const CancelToken& cancelled) {
    const fs::path staging = toStdPath(stagingDir);
    const fs::path game = toStdPath(gameDir);
    if (!stagingExists(staging)) {
        smm::fail(smm::ErrorCode::EnvironmentError,
                  "The staging directory does not exist: " + staging.string(), staging);
    }
    if (!stagingExists(game)) {
        smm::fail(smm::ErrorCode::EnvironmentError,
                  "The game directory does not exist: " + game.string(), game);
    }

    const auto outcome = smm::ModLoader::scan_mods_directory(staging);
    const auto plan = smm::DeploymentPlanner::build_plan("default", outcome.mods);
    const auto result = smm::execute_deploy(plan, smm::resolve_mods_dir(game),
                                            deployProgress(onProgress, cancelled));

    DeploySummary summary;
    summary.totalFiles = static_cast<int>(result.total_files);
    summary.hardLinks = static_cast<int>(result.hard_links_created);
    summary.copies = static_cast<int>(result.copied_files);
    summary.failed = static_cast<int>(result.failed_files);
    summary.bytesSaved = result.bytes_saved;
    summary.durationMs = result.duration_ms;
    summary.success = result.is_success();
    summary.warnings.reserve(static_cast<qsizetype>(result.warnings.size()));
    for (const auto& warning : result.warnings) {
        summary.warnings.append(QString::fromStdString(warning));
    }
    summary.errors.reserve(static_cast<qsizetype>(result.errors.size()));
    for (const auto& error : result.errors) {
        summary.errors.append(
            QString::fromStdString(error.first + ": " + error.second));
    }
    return summary;
}

RestoreSummary CoreService::restore(const QString& gameDir, const ProgressSink& onProgress,
                                    const CancelToken& cancelled) {
    const auto result = smm::restore_deploy(smm::resolve_mods_dir(toStdPath(gameDir)),
                                            deployProgress(onProgress, cancelled));
    RestoreSummary summary;
    summary.removedFiles = static_cast<int>(result.removed_files);
    summary.removedDirs = static_cast<int>(result.removed_dirs);
    summary.warnings.reserve(static_cast<qsizetype>(result.warnings.size()));
    for (const auto& warning : result.warnings) {
        summary.warnings.append(QString::fromStdString(warning));
    }
    return summary;
}

CoreService::EngineSetup CoreService::setupEngine(const QString& gameDir,
                                                  const QString& stagingDir) {
    const auto result =
        smm::provision_mod_engine(toStdPath(gameDir), toStdPath(stagingDir));
    EngineSetup setup;
    setup.hookPath = toQString(result.dinput8_path);
    setup.iniPath = toQString(result.ini_path);
    setup.installed = result.installed_dinput8;
    return setup;
}

// ---------------------------------------------------------------------------
// Maintenance
// ---------------------------------------------------------------------------

CoreService::PreviewOptimization CoreService::optimizePreviews(const QString& stagingDir) {
    const auto [count, saved] = smm::ModManager::optimize_all_previews(toStdPath(stagingDir));
    return PreviewOptimization{static_cast<int>(count), saved};
}

void CoreService::ensureStagingDir(const QString& stagingDir) {
    const fs::path staging = toStdPath(stagingDir);
    if (staging.empty()) {
        return;
    }
    std::error_code ec;
    fs::create_directories(staging, ec);
}

} // namespace smm::gui
