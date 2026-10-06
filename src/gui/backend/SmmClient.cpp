#include "SmmClient.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSettings>
#include <QThreadPool>
#include <algorithm>

#include <smm/conflict.hpp>
#include <smm/deploy.hpp>
#include <smm/doctor.hpp>
#include <smm/runtime_doctor.hpp>
#include <smm/error.hpp>
#include <smm/executor.hpp>
#include <smm/exporter.hpp>
#include <smm/importer.hpp>
#include <smm/loader.hpp>
#include <smm/manager.hpp>
#include <smm/preset.hpp>
#include <smm/types.hpp>

namespace smm::gui {

namespace {

fs::path toStdPath(const QString& qpath) {
    QString p = QDir::toNativeSeparators(qpath.trimmed());
#ifdef _WIN32
    return fs::path(p.toStdWString());
#else
    return fs::path(p.toStdString());
#endif
}

QString toQString(const fs::path& p) {
#ifdef _WIN32
    return QDir::toNativeSeparators(QString::fromStdWString(p.wstring()));
#else
    return QString::fromStdString(path_to_utf8(p));
#endif
}

ModEntry toModEntry(const smm::StagedMod& mod) {
    ModEntry e;
    e.id = QString::fromStdString(mod.info.id);
    e.name = QString::fromStdString(mod.info.name);
    e.version = QString::fromStdString(mod.info.version);
    e.author = QString::fromStdString(mod.info.author);
    e.category = QString::fromStdString(mod.info.category);
    e.description = mod.info.description ? QString::fromStdString(*mod.info.description) : QString{};
    e.homepage = mod.info.homepage ? QString::fromStdString(*mod.info.homepage) : QString{};
    e.sourceUrl = mod.info.source_url ? QString::fromStdString(*mod.info.source_url) : QString{};
    e.license = mod.info.license ? QString::fromStdString(*mod.info.license) : QString{};
    for (const auto& t : mod.info.tags) {
        e.tags.append(QString::fromStdString(t));
    }
    e.enabled = mod.info.enabled;
    e.priority = mod.info.priority;
    for (const auto& da : mod.info.disabled_assets) {
        e.disabledAssets.append(QString::fromStdString(da));
    }
    e.previewImage = mod.info.preview_image ? QString::fromStdString(*mod.info.preview_image) : QString{};
    e.assetCount = static_cast<int>(mod.assets.size());
    e.totalBytes = mod.total_bytes();
    e.rootPath = mod.info.root_path ? toQString(*mod.info.root_path) : QString{};
    return e;
}

AssetEntry toAssetEntry(const smm::AssetEntry& a) {
    AssetEntry ae;
    ae.relativePath = QString::fromStdString(a.relative_path);
    ae.category = QString::fromStdString(smm::to_string(a.category));
    ae.fileSize = a.file_size;
    ae.critical = a.is_critical;
    ae.exclusiveSlot = a.is_exclusive_slot;
    ae.enabled = a.enabled;
    return ae;
}

} // namespace

SmmClient::SmmClient(QObject* parent) : QObject(parent) {
    doctorPool_.setMaxThreadCount(1);
}

SmmClient::~SmmClient() { doctorPool_.waitForDone(); }

void SmmClient::setBusy(bool busy, const QString& label) {
    if (isBusy_ != busy) {
        isBusy_ = busy;
        emit busyChanged(isBusy_, label);
    }
}

void SmmClient::setStagingDir(const QString& dir) {
    stagingDir_ = dir;
}

void SmmClient::setGameDir(const QString& dir) {
    gameDir_ = dir;
}

void SmmClient::refreshEnvironment() {
    EnvironmentInfo info;
    info.settingsFile = QStringLiteral("QSettings:HKCU/Software/SekiroModManager/SMM");

    fs::path stagingPath = toStdPath(stagingDir_);
    if (stagingDir_.trimmed().isEmpty()) {
        stagingPath = smm::detect_staging_dir();
        if (!stagingPath.empty()) {
            stagingDir_ = toQString(stagingPath);
        }
    }
    std::error_code ec;
    if (!stagingPath.empty()) {
        fs::create_directories(stagingPath, ec);
    }
    info.stagingDir = stagingDir_;
    info.stagingExists = !stagingDir_.isEmpty() && fs::exists(toStdPath(stagingDir_));

    fs::path gamePath = toStdPath(gameDir_);
    if (gameDir_.trimmed().isEmpty()) {
        gamePath = smm::detect_game_dir();
        if (!gamePath.empty()) {
            gameDir_ = toQString(gamePath);
        }
    }
    info.gameDir = gameDir_;
    info.gameDirFound = !gameDir_.isEmpty();
    info.gameDirExists = info.gameDirFound && fs::exists(toStdPath(gameDir_));

    emit environmentLoaded(info);
}

void SmmClient::refreshMods() {
    refreshModState();
}

void SmmClient::refreshModState() {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        if (stagingDir_.isEmpty() || !fs::exists(stagingPath)) {
            emit modsLoaded({}, {}, stagingDir_);
            emit conflictsLoaded({});
            emit planLoaded({});
            return;
        }

        // One filesystem snapshot keeps the list, conflicts and plan consistent.
        const auto outcome = smm::ModLoader::scan_mods_directory(stagingPath);
        const auto report = smm::ConflictEngine::scan_conflicts(outcome.mods);
        const auto plan = smm::DeploymentPlanner::build_plan("default", outcome.mods);
        QVector<ModEntry> entries;
        entries.reserve(static_cast<qsizetype>(outcome.mods.size()));
        for (const auto& mod : outcome.mods) entries.append(toModEntry(mod));
        QStringList failures;
        for (const auto& failure : outcome.failures) {
            failures.append(toQString(failure.path) + QStringLiteral(": ") + QString::fromStdString(failure.message));
        }
        QVector<ConflictEntry> conflicts;
        conflicts.reserve(static_cast<qsizetype>(report.records.size()));
        for (const auto& conflict : report.records) {
            ConflictEntry entry;
            entry.relativePath = QString::fromStdString(conflict.relative_path);
            entry.severity = QString::fromStdString(smm::to_string(conflict.severity));
            entry.winnerModId = QString::fromStdString(conflict.winner_mod_id);
            for (const auto& shadowed : conflict.shadowed_mod_ids)
                entry.shadowedModIds.append(QString::fromStdString(shadowed));
            entry.message = QString::fromStdString(conflict.message);
            conflicts.append(entry);
        }
        PlanInfo info;
        info.profile = QString::fromStdString(plan.active_profile);
        info.fileCount = static_cast<int>(plan.mappings.size());
        info.mappings.reserve(static_cast<qsizetype>(plan.mappings.size()));
        for (const auto& mapping : plan.mappings) {
            PlanMapping entry;
            entry.targetRelativePath = QString::fromStdString(mapping.target_relative_path);
            entry.ownerModId = QString::fromStdString(mapping.owner_mod_id);
            entry.priority = mapping.priority;
            for (const auto& shadowed : mapping.shadowed_mods)
                entry.shadowedMods.append(QString::fromStdString(shadowed));
            info.mappings.append(entry);
        }
        info.totalConflicts = static_cast<int>(report.total_conflicts);
        info.criticalConflict = report.has_critical_conflict;
        info.warningConflict = report.has_warning_conflict;
        emit modsLoaded(entries, failures, stagingDir_);
        emit conflictsLoaded(conflicts);
        emit planLoaded(info);
    } catch (const std::exception& e) {
        const QString failure = QString::fromUtf8(e.what());
        emit modsLoaded({}, {failure}, stagingDir_);
        emit conflictsLoaded({});
        emit planLoaded({});
        emit operationFailed(tr("Mod Scan Failed"), failure);
    }
}

void SmmClient::refreshModDetail(const QString& modId) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    if (stagingDir_.isEmpty() || !fs::exists(stagingPath) || modId.isEmpty()) {
        return;
    }

    try {
        const auto mod = smm::ModManager::get_mod_details(stagingPath, modId.toStdString());
        const ModEntry me = toModEntry(mod);

        QVector<AssetEntry> assets;
        assets.reserve(static_cast<qsizetype>(mod.assets.size()));
        for (const auto& a : mod.assets) {
            assets.append(toAssetEntry(a));
        }

        emit modDetailLoaded(me, assets);
    } catch (const std::exception& e) {
        emit operationFailed(tr("Mod Detail Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::refreshConflicts() {
    refreshModState();
}

void SmmClient::refreshPlan() {
    refreshModState();
}

namespace {
HealthInfo toHealthInfo(const smm::HealthReport& health) {
    HealthInfo hi;
    hi.overall = QString::fromStdString(smm::to_string(health.overall));
    hi.gameDir = toQString(health.game_dir);
    hi.stagingDir = toQString(health.staging_dir);
    hi.okCount = static_cast<int>(health.ok_count);
    hi.infoCount = static_cast<int>(health.info_count);
    hi.warningCount = static_cast<int>(health.warning_count);
    hi.errorCount = static_cast<int>(health.error_count);
    for (const auto& it : health.items) {
        hi.items.append({QString::fromStdString(it.id), QString::fromStdString(it.category),
                         QString::fromStdString(it.title), QString::fromStdString(it.detail),
                         it.remediation ? QString::fromStdString(*it.remediation) : QString{},
                         QString::fromStdString(smm::to_string(it.status))});
    }
    return hi;
}

HealthInfo withRuntime(HealthInfo base) {
    const auto runtime = toHealthInfo(smm::diagnose_launch_runtime(
        smm::inspect_launch_runtime(toStdPath(base.gameDir))));
    base.items += runtime.items;
    base.okCount += runtime.okCount;
    base.infoCount += runtime.infoCount;
    base.warningCount += runtime.warningCount;
    base.errorCount += runtime.errorCount;
    base.overall = base.errorCount ? QStringLiteral("action_required") :
                   base.warningCount ? QStringLiteral("degraded") : QStringLiteral("healthy");
    const auto rank = [](const QString& status) {
        return status == QLatin1String("error") ? 0 : status == QLatin1String("warning") ? 1 :
               status == QLatin1String("info") ? 2 : 3;
    };
    std::stable_sort(base.items.begin(), base.items.end(), [&rank](const auto& a, const auto& b) {
        return rank(a.status) < rank(b.status);
    });
    return base;
}
}

HealthInfo SmmClient::collectHealth(const QString& game, const QString& staging) {
    try {
        return toHealthInfo(smm::diagnose_environment(toStdPath(game), toStdPath(staging), true));
    } catch (const std::exception& error) {
        HealthInfo failed;
        failed.gameDir = game;
        failed.stagingDir = staging;
        failed.overall = QStringLiteral("action_required");
        failed.errorCount = 1;
        failed.items.append({QStringLiteral("doctor-failed"), QStringLiteral("diagnostic"),
            tr("诊断未完成"), QString::fromUtf8(error.what()), tr("请检查目录访问权限后重新检测。"),
            QStringLiteral("error")});
        return failed;
    }
}

void SmmClient::refreshDoctor() {
    ++doctorGeneration_;
    if (!doctorRunning_) runDoctor();
}

void SmmClient::runDoctor() {
    doctorRunning_ = true;
    emit doctorCheckingChanged();
    const auto generation = doctorGeneration_;
    const auto game = gameDir_;
    const auto staging = stagingDir_;
    doctorPool_.start([this, generation, game, staging]() {
        const auto base = collectHealth(game, staging);
        QMetaObject::invokeMethod(this, [this, generation, base]() {
            if (generation != doctorGeneration_) { runDoctor(); return; }
            baseHealth_ = base;
            // Runtime is observed in the GUI process after the filesystem check finishes.
            emit doctorLoaded(withRuntime(baseHealth_));
            doctorRunning_ = false;
            emit doctorCheckingChanged();
        }, Qt::QueuedConnection);
    });
}

void SmmClient::refreshRuntimeDoctor() {
    if (!doctorRunning_ && !baseHealth_.overall.isEmpty()) emit doctorLoaded(withRuntime(baseHealth_));
}

void SmmClient::refreshPresets() {
    const fs::path stagingPath = toStdPath(stagingDir_);
    if (stagingDir_.isEmpty() || !fs::exists(stagingPath)) {
        emit presetsLoaded({});
        return;
    }

    try {
        const auto presets = smm::PresetManager::list_presets(stagingPath);
        QVector<PresetEntry> entries;
        entries.reserve(static_cast<qsizetype>(presets.size()));
        for (const auto& p : presets) {
            PresetEntry pe;
            pe.id = QString::fromStdString(p.id);
            pe.name = QString::fromStdString(p.name);
            pe.description = p.description ? QString::fromStdString(*p.description) : QString{};
            pe.nameEn = p.name_en ? QString::fromStdString(*p.name_en) : QString{};
            pe.descriptionEn = p.description_en ? QString::fromStdString(*p.description_en) : QString{};
            pe.modCount = static_cast<int>(p.mods.size());
            pe.updatedAt = p.updated_at;
            entries.append(pe);
        }

        emit presetsLoaded(entries);
    } catch (const std::exception& e) {
        emit operationFailed(tr("Preset Scan Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::setModEnabled(const QString& modId, bool enabled) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::set_mod_enabled(stagingPath, modId.toStdString(), enabled);
        emit operationSucceeded(tr("Mod Updated"), tr("%1 is now %2").arg(modId, enabled ? QStringLiteral("enabled") : QStringLiteral("disabled")));
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Update Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::setModsEnabled(const QStringList& modIds, bool enabled) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    int updated = 0;
    QStringList uniqueIds = modIds;
    uniqueIds.removeDuplicates();
    for (const QString& id : uniqueIds) {
        try {
            smm::ModManager::set_mod_enabled(stagingPath, id.toStdString(), enabled);
            ++updated;
        } catch (const std::exception& e) {
            emit operationFailed(tr("Update Failed"), id + QStringLiteral(": ") + QString::fromUtf8(e.what()));
        }
    }
    if (updated > 0) {
        refreshModState();
        emit operationSucceeded(tr("Mod Updated"), tr("Updated %1 mods.").arg(updated));
    }
}

void SmmClient::removeMods(const QStringList& modIds) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    int removed = 0;
    QStringList uniqueIds = modIds;
    uniqueIds.removeDuplicates();
    for (const QString& id : uniqueIds) {
        try {
            smm::ModManager::delete_mod(stagingPath, id.toStdString());
            ++removed;
        } catch (const std::exception& e) {
            emit operationFailed(tr("Delete Failed"), id + QStringLiteral(": ") + QString::fromUtf8(e.what()));
        }
    }
    if (removed > 0) {
        refreshModState();
        emit operationSucceeded(tr("Mod Deleted"), tr("Deleted %1 mods successfully.").arg(removed));
    }
}

void SmmClient::setAssetEnabled(const QString& modId, const QString& relPath, bool enabled) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::set_asset_enabled(stagingPath, modId.toStdString(), relPath.toStdString(), enabled);
        emit operationSucceeded(tr("Asset Updated"), tr("%1 asset updated").arg(relPath));
        refreshModDetail(modId);
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Asset Update Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::setModPreview(const QString& modId, const QString& imagePath) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::set_mod_preview(stagingPath, modId.toStdString(), toStdPath(imagePath));
        emit operationSucceeded(tr("Preview Updated"), tr("Preview background set for %1").arg(modId));
        refreshModDetail(modId);
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Preview Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::setModPriority(const QString& modId, quint32 priority) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::set_mod_priority(stagingPath, modId.toStdString(), priority);
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Priority Update Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::swapModPriorities(const QString& modIdA, quint32 priA, const QString& modIdB, quint32 priB) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::set_mod_priority(stagingPath, modIdA.toStdString(), priA);
        smm::ModManager::set_mod_priority(stagingPath, modIdB.toStdString(), priB);
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Priority Swap Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::batchUpdatePriorities(const QMap<QString, quint32>& priorities) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        for (auto it = priorities.constBegin(); it != priorities.constEnd(); ++it) {
            smm::ModManager::set_mod_priority(stagingPath, it.key().toStdString(), it.value());
        }
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Batch Priority Update Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::removeMod(const QString& modId) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::ModManager::delete_mod(stagingPath, modId.toStdString());
        emit operationSucceeded(tr("Mod Deleted"), tr("%1 has been removed").arg(modId));
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Delete Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::exportSingleMod(const QString& modId, const QString& outputPath) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    fs::path out = toStdPath(outputPath);
    if (out.empty()) {
        const auto exportDir = stagingPath / "exports";
        fs::create_directories(exportDir);
        out = exportDir / (modId.toStdString() + ".zip");
    }
    try {
        const auto res = smm::export_single_mod(stagingPath, modId.toStdString(), out);
        emit operationSucceeded(tr("Mod Exported"), tr("Saved to %1").arg(toQString(res)));
    } catch (const std::exception& e) {
        emit operationFailed(tr("Export Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::updateModMetadata(const QString& modId, const QString& newId,
                                  const QString& name, const QString& author,
                                  const QString& version, const QString& category,
                                  const QString& description, const QString& sourceUrl) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        const auto modDir = smm::ModManager::find_mod_dir(stagingPath, modId.toStdString());
        auto info = smm::ModLoader::load_mod_info(modDir);
        if (!name.trimmed().isEmpty()) info.name = name.trimmed().toStdString();
        if (!author.trimmed().isEmpty()) info.author = author.trimmed().toStdString();
        if (!version.trimmed().isEmpty()) info.version = version.trimmed().toStdString();
        if (!category.trimmed().isEmpty()) info.category = category.trimmed().toStdString();
        info.description = description.toStdString();
        info.source_url = sourceUrl.toStdString();

        QString targetId = newId.trimmed();
        if (targetId.isEmpty()) {
            targetId = modId;
        } else {
            targetId.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("-"));
        }

        fs::path finalModDir = modDir;
        if (targetId != modId) {
            fs::path targetDir = stagingPath / toStdPath(targetId);
            std::error_code ec;
            if (fs::exists(targetDir, ec)) {
                throw std::runtime_error(tr("Mod ID '%1' already exists.").arg(targetId).toStdString());
            }
            fs::rename(modDir, targetDir, ec);
            if (!ec) {
                finalModDir = targetDir;
                info.id = targetId.toStdString();
            } else {
                info.id = targetId.toStdString();
            }
        }

        smm::ModManager::save_mod_info(finalModDir, info);
        emit operationSucceeded(tr("Metadata Saved"), tr("Mod details updated."));
        refreshModDetail(targetId);
        refreshModState();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Save Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::savePreset(const QString& name, const QString& description,
                           const QString& nameEn, const QString& descriptionEn) {
    fs::path stagingPath = toStdPath(stagingDir_);
    try {
        if (stagingDir_.isEmpty() || !fs::exists(stagingPath)) {
            stagingPath = smm::detect_staging_dir(stagingPath);
            std::error_code ec;
            fs::create_directories(stagingPath, ec);
            stagingDir_ = toQString(stagingPath);
        }
        auto preset = smm::PresetManager::create_preset_from_current(
            stagingPath, name.toStdString(), description.toStdString());
        if (!nameEn.trimmed().isEmpty()) {
            preset.name_en = nameEn.trimmed().toStdString();
        }
        if (!descriptionEn.trimmed().isEmpty()) {
            preset.description_en = descriptionEn.trimmed().toStdString();
        }
        smm::PresetManager::save_preset(stagingPath, preset);

        emit operationSucceeded(tr("Preset Saved"), tr("Preset '%1' created.").arg(name));
        refreshPresets();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Preset Save Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::applyPreset(const QString& presetId) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        const auto p = smm::PresetManager::apply_preset(stagingPath, presetId.toStdString());
        emit operationSucceeded(tr("Preset Applied"), tr("Preset '%1' equipped.").arg(QString::fromStdString(p.name)));
        refreshModState();
        refreshPresets();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Preset Apply Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::deletePreset(const QString& presetId) {
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        smm::PresetManager::delete_preset(stagingPath, presetId.toStdString());
        emit operationSucceeded(tr("Preset Deleted"), tr("Preset removed."));
        refreshPresets();
    } catch (const std::exception& e) {
        emit operationFailed(tr("Preset Delete Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::setupEngine() {
    const fs::path gamePath = toStdPath(gameDir_);
    const fs::path stagingPath = toStdPath(stagingDir_);
    try {
        const auto res = smm::provision_mod_engine(gamePath, stagingPath);
        emit engineConfigured(toQString(res.dinput8_path), toQString(res.ini_path), res.installed_dinput8);
        emit operationSucceeded(tr("ModEngine Setup"), tr("ModEngine hook and ini successfully configured."));
    } catch (const std::exception& e) {
        emit operationFailed(tr("ModEngine Setup Failed"), QString::fromUtf8(e.what()));
    }
}

void SmmClient::saveConfig(const QString& staging, const QString& game) {
    stagingDir_ = staging.trimmed();
    gameDir_ = game.trimmed();

    std::error_code ec;
    if (!stagingDir_.isEmpty()) {
        fs::create_directories(toStdPath(stagingDir_), ec);
    }

    QSettings settings(QStringLiteral("SekiroModManager"), QStringLiteral("SMM"));
    settings.setValue(QStringLiteral("stagingDir"), stagingDir_);
    settings.setValue(QStringLiteral("sekiroDir"), gameDir_);
    settings.sync();

    emit operationSucceeded(tr("Settings Saved"), tr("Configuration successfully persisted."));
    refreshEnvironment();
}

void SmmClient::deploy() {
    setBusy(true, tr("Deploying mods..."));
    const fs::path stagingPath = toStdPath(stagingDir_);
    const fs::path gamePath = toStdPath(gameDir_);

    QThreadPool::globalInstance()->start([this, stagingPath, gamePath]() {
        try {
            if (!fs::exists(stagingPath)) {
                throw std::runtime_error("Staging directory does not exist.");
            }
            if (!fs::exists(gamePath)) {
                throw std::runtime_error("Game directory does not exist.");
            }

            fs::path targetModsDir = gamePath;
            if (targetModsDir.filename() != "mods") {
                targetModsDir /= "mods";
            }

            const auto outcome = smm::ModLoader::scan_mods_directory(stagingPath);
            const auto plan = smm::DeploymentPlanner::build_plan("default", outcome.mods);

            const auto deployResult = smm::execute_deploy(plan, targetModsDir, [this](const smm::DeployProgress& p) {
                QMetaObject::invokeMethod(this, [this, p]() {
                    emit progress(tr("Deploying"), static_cast<int>(p.done), static_cast<int>(p.total),
                                  QString::fromStdString(p.current_path));
                }, Qt::QueuedConnection);
            });

            DeploySummary ds;
            ds.totalFiles = static_cast<int>(deployResult.total_files);
            ds.hardLinks = static_cast<int>(deployResult.hard_links_created);
            ds.copies = static_cast<int>(deployResult.copied_files);
            ds.failed = static_cast<int>(deployResult.failed_files);
            ds.bytesSaved = deployResult.bytes_saved;
            ds.durationMs = deployResult.duration_ms;
            for (const auto& w : deployResult.warnings) {
                ds.warnings.append(QString::fromStdString(w));
            }
            for (const auto& err : deployResult.errors) {
                ds.errors.append(QString::fromStdString(err.first + ": " + err.second));
            }
            ds.success = deployResult.is_success();

            PlanInfo pi;
            pi.profile = QString::fromStdString(plan.active_profile);
            pi.fileCount = static_cast<int>(plan.mappings.size());

            QMetaObject::invokeMethod(this, [this, ds, pi]() {
                setBusy(false);
                emit deployFinished(ds, pi);
            }, Qt::QueuedConnection);

        } catch (const std::exception& e) {
            const QString err = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, err]() {
                setBusy(false);
                emit operationFailed(tr("Deploy Failed"), err);
            }, Qt::QueuedConnection);
        }
    });
}

void SmmClient::restore() {
    setBusy(true, tr("Restoring clean game state..."));
    const fs::path gamePath = toStdPath(gameDir_);

    QThreadPool::globalInstance()->start([this, gamePath]() {
        try {
            fs::path targetModsDir = gamePath;
            if (targetModsDir.filename() != "mods") {
                targetModsDir /= "mods";
            }

            const auto restoreResult = smm::restore_deploy(targetModsDir, [this](const smm::DeployProgress& p) {
                QMetaObject::invokeMethod(this, [this, p]() {
                    emit progress(tr("Restoring"), static_cast<int>(p.done), static_cast<int>(p.total),
                                  QString::fromStdString(p.current_path));
                }, Qt::QueuedConnection);
            });

            RestoreSummary rs;
            rs.removedFiles = static_cast<int>(restoreResult.removed_files);
            rs.removedDirs = static_cast<int>(restoreResult.removed_dirs);
            for (const auto& w : restoreResult.warnings) {
                rs.warnings.append(QString::fromStdString(w));
            }

            QMetaObject::invokeMethod(this, [this, rs]() {
                setBusy(false);
                emit restoreFinished(rs);
            }, Qt::QueuedConnection);

        } catch (const std::exception& e) {
            const QString err = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, err]() {
                setBusy(false);
                emit operationFailed(tr("Restore Failed"), err);
            }, Qt::QueuedConnection);
        }
    });
}

void SmmClient::importPaths(const QStringList& paths, const QString& customId, const QString& customName,
                            bool overwrite) {
    if (paths.isEmpty()) return;
    setBusy(true, tr("Importing mods..."));
    const fs::path stagingPath = toStdPath(stagingDir_);

    QThreadPool::globalInstance()->start([this, paths, customId, customName, overwrite, stagingPath]() {
        try {
            smm::ImportOptions opts;
            if (!customId.isEmpty()) opts.id = customId.toStdString();
            if (!customName.isEmpty()) opts.name = customName.toStdString();
            opts.overwrite = overwrite;

            for (const auto& p : paths) {
                const fs::path src = toStdPath(p);
                const auto res = smm::import_mod(src, stagingPath, opts);

                ImportSummary summary;
                summary.id = QString::fromStdString(res.info.id);
                summary.name = QString::fromStdString(res.info.name);
                summary.version = QString::fromStdString(res.info.version);
                summary.category = QString::fromStdString(res.info.category);
                summary.priority = res.info.priority;
                summary.assetCount = static_cast<int>(res.asset_count);
                summary.ignoredCount = static_cast<int>(res.ignored_count);
                summary.totalBytes = res.total_bytes;
                summary.replacedExisting = res.replaced_existing;
                summary.dryRun = res.dry_run;
                summary.location = toQString(res.mod_dir);

                QMetaObject::invokeMethod(this, [this, summary]() {
                    emit importFinished(summary);
                }, Qt::QueuedConnection);
            }

            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false);
                emit operationSucceeded(tr("Import Complete"), tr("All mods imported successfully."));
                refreshModState();
            }, Qt::QueuedConnection);

        } catch (const std::exception& e) {
            const QString err = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, err]() {
                setBusy(false);
                emit operationFailed(tr("Import Failed"), err);
            }, Qt::QueuedConnection);
        }
    });
}

void SmmClient::exportModPack(const QStringList& modIds, const QString& outputPath, const QString& name,
                              const QString& description) {
    setBusy(true, tr("Exporting mod pack..."));
    const fs::path stagingPath = toStdPath(stagingDir_);
    const fs::path outPath = outputPath.isEmpty()
        ? (stagingPath / (name.toStdString() + ".smmpack"))
        : toStdPath(outputPath);

    std::vector<std::string> ids;
    ids.reserve(modIds.size());
    for (const auto& id : modIds) {
        ids.push_back(id.toStdString());
    }

    QThreadPool::globalInstance()->start([this, stagingPath, ids, outPath, name, description]() {
        try {
            smm::export_modpack(stagingPath, ids, outPath, name.toStdString(), description.toStdString(),
                                {}, [this](std::size_t done, std::size_t total, const std::string& current) {
                QMetaObject::invokeMethod(this, [this, done, total, current]() {
                    emit progress(tr("Exporting"), static_cast<int>(done), static_cast<int>(total),
                                  QString::fromStdString(current));
                }, Qt::QueuedConnection);
            });

            QMetaObject::invokeMethod(this, [this, outPath]() {
                setBusy(false);
                emit operationSucceeded(tr("Export Succeeded"),
                                        tr("Saved pack to %1").arg(toQString(outPath)));
            }, Qt::QueuedConnection);
        } catch (const std::exception& e) {
            const QString err = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, err]() {
                setBusy(false);
                emit operationFailed(tr("Export Failed"), err);
            }, Qt::QueuedConnection);
        }
    });
}

void SmmClient::importModPack(const QString& packPath) {
    setBusy(true, tr("Importing mod pack..."));
    const fs::path stagingPath = toStdPath(stagingDir_);
    const fs::path file = toStdPath(packPath);

    QThreadPool::globalInstance()->start([this, file, stagingPath]() {
        try {
            const auto res = smm::import_modpack(file, stagingPath, true);

            // 将导入的整合包同步写入预设配置（.smm_presets.json），确保在“整合包预设”界面立即展示并可一键装配
            smm::ModPreset preset;
            preset.name = res.manifest.name;
            if (res.manifest.description) {
                preset.description = *res.manifest.description;
            }
            for (const auto& item : res.manifest.mods) {
                preset.mods.push_back(smm::ModPresetEntry{item.id, item.priority});
            }
            smm::PresetManager::save_preset(stagingPath, preset);

            QMetaObject::invokeMethod(this, [this, res]() {
                setBusy(false);
                emit operationSucceeded(tr("Pack Imported"),
                                        tr("Imported '%1' with %2 mods.")
                                            .arg(QString::fromStdString(res.manifest.name))
                                            .arg(res.mods.size()));
                refreshModState();
                refreshPresets();
            }, Qt::QueuedConnection);
        } catch (const std::exception& e) {
            const QString err = QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, err]() {
                setBusy(false);
                emit operationFailed(tr("Pack Import Failed"), err);
            }, Qt::QueuedConnection);
        }
    });
}

} // namespace smm::gui
