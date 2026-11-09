#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

/// Plain data shared by the GUI adapters, models and controller.
///
/// These are Qt-shaped projections of `smm_core` results: they carry display strings and
/// integers, never core objects, so a QAbstractListModel never sees a core type and the
/// core never sees a Qt type.
namespace smm::gui {

/// One staged mod, with full metadata and assets summary.
struct ModEntry {
    QString id;
    QString name;
    QString version;
    QString author;
    QString category;
    QString description;
    QString homepage;
    QString sourceUrl;
    QString license;
    QStringList tags;
    bool enabled{true};
    quint32 priority{100};
    QStringList disabledAssets;
    QString previewImage;
    int assetCount{0};
    quint64 totalBytes{0};
    QString rootPath;
};

struct AssetEntry {
    QString relativePath;
    QString category;
    quint64 fileSize{0};
    bool critical{false};
    bool exclusiveSlot{false};
    bool enabled{true};
};

struct ConflictEntry {
    QString relativePath;
    QString severity; ///< "info" | "warning" | "critical"
    QString winnerModId;
    QStringList shadowedModIds;
    QString message;
};

struct PlanMapping {
    QString targetRelativePath;
    QString ownerModId;
    quint32 priority{100};
    QStringList shadowedMods;
};

struct PlanInfo {
    QString profile;
    QString targetDir;
    int fileCount{0};
    QVector<PlanMapping> mappings;
    QVector<ConflictEntry> conflicts;
    bool criticalConflict{false};
    bool warningConflict{false};
    int totalConflicts{0};
};

struct DeploySummary {
    int totalFiles{0};
    int hardLinks{0};
    int copies{0};
    int failed{0};
    quint64 bytesSaved{0};
    quint64 durationMs{0};
    QStringList warnings;
    QStringList errors;
    bool success{true};
};

struct RestoreSummary {
    int removedFiles{0};
    int removedDirs{0};
    QStringList warnings;
};

struct DiagnosticEntry {
    QString id;
    QString category;
    QString title;
    QString detail;
    QString remediation;
    QString status; ///< "ok" | "info" | "warning" | "error"
};

struct HealthInfo {
    QString overall; ///< "healthy" | "degraded" | "action_required"
    QString gameDir;
    QString stagingDir;
    int okCount{0};
    int infoCount{0};
    int warningCount{0};
    int errorCount{0};
    QVector<DiagnosticEntry> items;
};

struct PresetEntry {
    QString id;
    QString name;
    QString description;
    QString nameEn;
    QString descriptionEn;
    int modCount{0};
    quint64 updatedAt{0};
};

struct EnvironmentInfo {
    QString settingsFile;
    QString stagingDir;
    bool stagingExists{false};
    QString gameDir;
    bool gameDirFound{false};
    bool gameDirExists{false};
};

struct ImportSummary {
    QString id;
    QString name;
    QString version;
    QString category;
    quint32 priority{100};
    int assetCount{0};
    int ignoredCount{0};
    quint64 totalBytes{0};
    bool replacedExisting{false};
    bool dryRun{false};
    QString location;
};

/// Result of importing a whole .smmpack container.
struct ModPackImportSummary {
    QString name;
    int modCount{0};
};

/// One filesystem snapshot of the staging directory.
///
/// The mod list, the conflict report and the deployment plan come from the same scan so a
/// refresh can never show a mod list that disagrees with the plan next to it.
struct StagingSnapshot {
    bool scanned{false};
    QVector<ModEntry> mods;
    QStringList failures;
    QVector<ConflictEntry> conflicts;
    PlanInfo plan;
};

} // namespace smm::gui
