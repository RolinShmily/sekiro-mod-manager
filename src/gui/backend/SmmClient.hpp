#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

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

/// High-performance native client: directly links and invokes smm_core C++ APIs.
/// Eliminates subprocess IPC, JSON parsing, and process overhead.
class SmmClient : public QObject {
    Q_OBJECT

public:
    explicit SmmClient(QObject* parent = nullptr);

    static QString locateCli() { return QStringLiteral("embedded:smm_core"); }

    bool isAvailable() const { return true; }
    QString executable() const { return QStringLiteral("embedded:smm_core"); }
    QString resolvedStagingDir() const { return stagingDir_; }
    QString resolvedGameDir() const { return gameDir_; }
    bool isBusy() const { return isBusy_; }

    void setStagingDir(const QString& dir);
    void setGameDir(const QString& dir);

    // Reads
    void refreshEnvironment();
    void refreshMods();
    void refreshModDetail(const QString& modId);
    void refreshConflicts();
    void refreshPlan();
    void refreshDoctor();
    void refreshPresets();

    // Writes
    void setModEnabled(const QString& modId, bool enabled);
    void setAssetEnabled(const QString& modId, const QString& relPath, bool enabled);
    void setModPreview(const QString& modId, const QString& imagePath);
    void setModPriority(const QString& modId, quint32 priority);
    void swapModPriorities(const QString& modIdA, quint32 priA, const QString& modIdB, quint32 priB);
    void batchUpdatePriorities(const QMap<QString, quint32>& priorities);
    void removeMod(const QString& modId);
    void exportSingleMod(const QString& modId, const QString& outputPath = {});
    void updateModMetadata(const QString& modId, const QString& name, const QString& author,
                           const QString& version, const QString& category,
                           const QString& description, const QString& sourceUrl);
    void importPaths(const QStringList& paths, const QString& customId, const QString& customName,
                     bool overwrite);
    void exportModPack(const QStringList& modIds, const QString& outputPath, const QString& name,
                       const QString& description);
    void importModPack(const QString& packPath);
    void deploy();
    void restore();
    void setupEngine();
    void savePreset(const QString& name, const QString& description);
    void applyPreset(const QString& presetId);
    void deletePreset(const QString& presetId);
    void saveConfig(const QString& stagingDir, const QString& gameDir);

signals:
    void environmentLoaded(const smm::gui::EnvironmentInfo& environment);
    void modsLoaded(const QVector<smm::gui::ModEntry>& mods, const QStringList& failures,
                    const QString& stagingDir);
    void modDetailLoaded(const smm::gui::ModEntry& mod, const QVector<smm::gui::AssetEntry>& assets);
    void conflictsLoaded(const QVector<smm::gui::ConflictEntry>& conflicts);
    void planLoaded(const smm::gui::PlanInfo& plan);
    void doctorLoaded(const smm::gui::HealthInfo& health);
    void presetsLoaded(const QVector<smm::gui::PresetEntry>& presets);

    void deployFinished(const smm::gui::DeploySummary& summary, const smm::gui::PlanInfo& plan);
    void restoreFinished(const smm::gui::RestoreSummary& summary);
    void engineConfigured(const QString& hookPath, const QString& iniPath, bool installed);
    void importFinished(const smm::gui::ImportSummary& summary);

    void operationSucceeded(const QString& title, const QString& detail);
    void operationFailed(const QString& title, const QString& message);

    void progress(const QString& phase, int done, int total, const QString& current);
    void busyChanged(bool busy, const QString& label);

private:
    void setBusy(bool busy, const QString& label = {});

    QString stagingDir_;
    QString gameDir_;
    bool isBusy_{false};
};

} // namespace smm::gui
