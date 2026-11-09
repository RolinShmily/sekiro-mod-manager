#include "backend/adapters/CoreMapping.hpp"

#include <QDir>

namespace smm::gui {

fs::path toStdPath(const QString& qpath) {
    const QString native = QDir::toNativeSeparators(qpath.trimmed());
#ifdef _WIN32
    return fs::path(native.toStdWString());
#else
    return fs::path(native.toStdString());
#endif
}

QString toQString(const fs::path& path) {
    if (path.empty()) {
        return {};
    }
#ifdef _WIN32
    return QDir::toNativeSeparators(QString::fromStdWString(path.wstring()));
#else
    return QString::fromStdString(smm::path_to_utf8(path));
#endif
}

ModEntry toModEntry(const smm::StagedMod& mod) {
    ModEntry entry;
    entry.id = QString::fromStdString(mod.info.id);
    entry.name = QString::fromStdString(mod.info.name);
    entry.version = QString::fromStdString(mod.info.version);
    entry.author = QString::fromStdString(mod.info.author);
    entry.category = QString::fromStdString(mod.info.category);
    entry.description =
        mod.info.description ? QString::fromStdString(*mod.info.description) : QString{};
    entry.homepage = mod.info.homepage ? QString::fromStdString(*mod.info.homepage) : QString{};
    entry.sourceUrl = mod.info.source_url ? QString::fromStdString(*mod.info.source_url) : QString{};
    entry.license = mod.info.license ? QString::fromStdString(*mod.info.license) : QString{};
    entry.tags.reserve(static_cast<qsizetype>(mod.info.tags.size()));
    for (const auto& tag : mod.info.tags) {
        entry.tags.append(QString::fromStdString(tag));
    }
    entry.enabled = mod.info.enabled;
    entry.priority = mod.info.priority;
    entry.disabledAssets.reserve(static_cast<qsizetype>(mod.info.disabled_assets.size()));
    for (const auto& asset : mod.info.disabled_assets) {
        entry.disabledAssets.append(QString::fromStdString(asset));
    }
    entry.previewImage =
        mod.info.preview_image ? QString::fromStdString(*mod.info.preview_image) : QString{};
    entry.assetCount = static_cast<int>(mod.assets.size());
    entry.totalBytes = mod.total_bytes();
    entry.rootPath = mod.info.root_path ? toQString(*mod.info.root_path) : QString{};
    return entry;
}

AssetEntry toAssetEntry(const smm::AssetEntry& asset) {
    AssetEntry entry;
    entry.relativePath = QString::fromStdString(asset.relative_path);
    entry.category = QString::fromStdString(smm::to_string(asset.category));
    entry.fileSize = asset.file_size;
    entry.critical = asset.is_critical;
    entry.exclusiveSlot = asset.is_exclusive_slot;
    entry.enabled = asset.enabled;
    return entry;
}

ConflictEntry toConflictEntry(const smm::ConflictRecord& record) {
    ConflictEntry entry;
    entry.relativePath = QString::fromStdString(record.relative_path);
    entry.severity = QString::fromStdString(smm::to_string(record.severity));
    entry.winnerModId = QString::fromStdString(record.winner_mod_id);
    entry.shadowedModIds.reserve(static_cast<qsizetype>(record.shadowed_mod_ids.size()));
    for (const auto& shadowed : record.shadowed_mod_ids) {
        entry.shadowedModIds.append(QString::fromStdString(shadowed));
    }
    entry.message = QString::fromStdString(record.message);
    return entry;
}

PlanInfo toPlanInfo(const smm::DeployPlan& plan) {
    PlanInfo info;
    info.profile = QString::fromStdString(plan.active_profile);
    info.fileCount = static_cast<int>(plan.mappings.size());
    info.mappings.reserve(static_cast<qsizetype>(plan.mappings.size()));
    for (const auto& mapping : plan.mappings) {
        PlanMapping entry;
        entry.targetRelativePath = QString::fromStdString(mapping.target_relative_path);
        entry.ownerModId = QString::fromStdString(mapping.owner_mod_id);
        entry.priority = mapping.priority;
        entry.shadowedMods.reserve(static_cast<qsizetype>(mapping.shadowed_mods.size()));
        for (const auto& shadowed : mapping.shadowed_mods) {
            entry.shadowedMods.append(QString::fromStdString(shadowed));
        }
        info.mappings.append(entry);
    }
    info.criticalConflict = plan.conflict_report.has_critical_conflict;
    info.warningConflict = plan.conflict_report.has_warning_conflict;
    info.totalConflicts = static_cast<int>(plan.conflict_report.total_conflicts);
    return info;
}

HealthInfo toHealthInfo(const smm::HealthReport& report) {
    HealthInfo info;
    info.overall = QString::fromStdString(smm::to_string(report.overall));
    info.gameDir = toQString(report.game_dir);
    info.stagingDir = toQString(report.staging_dir);
    info.okCount = static_cast<int>(report.ok_count);
    info.infoCount = static_cast<int>(report.info_count);
    info.warningCount = static_cast<int>(report.warning_count);
    info.errorCount = static_cast<int>(report.error_count);
    info.items.reserve(static_cast<qsizetype>(report.items.size()));
    for (const auto& item : report.items) {
        info.items.append(DiagnosticEntry{
            QString::fromStdString(item.id), QString::fromStdString(item.category),
            QString::fromStdString(item.title), QString::fromStdString(item.detail),
            item.remediation ? QString::fromStdString(*item.remediation) : QString{},
            QString::fromStdString(smm::to_string(item.status))});
    }
    return info;
}

std::vector<std::string> toIdVector(const QStringList& ids) {
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(ids.size()));
    for (const auto& id : ids) {
        if (id.isEmpty()) {
            continue;
        }
        if (std::find(result.begin(), result.end(), id.toStdString()) != result.end()) {
            continue;
        }
        result.push_back(id.toStdString());
    }
    return result;
}

} // namespace smm::gui
