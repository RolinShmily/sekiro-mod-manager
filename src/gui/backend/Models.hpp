#pragma once

#include <QAbstractListModel>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonObject>
#include <QObject>
#include <QUrl>
#include <QVector>

#include "SmmClient.hpp"
#include "UpdateManager.hpp"

namespace smm::gui {

/// QML-friendly List Model for staged mods.
class ModListModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString selectedCategory READ selectedCategory WRITE setSelectedCategory NOTIFY selectedCategoryChanged)
    Q_PROPERTY(QStringList availableCategories READ availableCategories NOTIFY availableCategoriesChanged)
    Q_PROPERTY(bool onlyEquippedPackMods READ onlyEquippedPackMods WRITE setOnlyEquippedPackMods NOTIFY onlyEquippedPackModsChanged)

public:
    enum ModRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        VersionRole,
        AuthorRole,
        CategoryRole,
        DescriptionRole,
        HomepageRole,
        SourceUrlRole,
        EnabledRole,
        PriorityRole,
        AssetCountRole,
        TotalBytesRole,
        RootPathRole,
        PreviewImageRole,
        PreviewImagePathRole,
        DisabledAssetsRole,
        RankRole
    };

    explicit ModListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }
    QString filterText() const { return filterText_; }
    void setFilterText(const QString& filter);

    QString selectedCategory() const { return selectedCategory_; }
    void setSelectedCategory(const QString& cat);

    QStringList availableCategories() const { return availableCategories_; }

    bool onlyEquippedPackMods() const { return onlyEquippedPackMods_; }
    void setOnlyEquippedPackMods(bool only);
    void setEquippedModIds(const QSet<QString>& ids);

    void setMods(const QVector<ModEntry>& mods);
    const QVector<ModEntry>& mods() const { return mods_; }
    const ModEntry* findMod(const QString& id) const;

    Q_INVOKABLE void toggleEnabled(int row);
    Q_INVOKABLE void setPriority(int row, quint32 priority);
    Q_INVOKABLE void swapPriority(int rowA, int rowB);
    Q_INVOKABLE void moveRow(int from, int to);
    Q_INVOKABLE QStringList allModIds() const {
        QStringList list;
        for (const auto& m : mods_) list.append(m.id);
        return list;
    }

signals:
    void countChanged();
    void filterTextChanged();
    void selectedCategoryChanged();
    void availableCategoriesChanged();
    void onlyEquippedPackModsChanged();
    void modToggled(const QString& modId, bool enabled);
    void priorityChanged(const QString& modId, quint32 priority);
    void prioritiesSwapped(const QString& modIdA, quint32 priA, const QString& modIdB, quint32 priB);
    void batchPrioritiesChanged(const QMap<QString, quint32>& priorities);

private:
    void rebuildFilter();
    QVector<int> filteredIndices(const QVector<ModEntry>& mods) const;

    QVector<ModEntry> mods_;
    QVector<int> visibleIndices_;
    QString filterText_;
    QString selectedCategory_{QStringLiteral("all")};
    QStringList availableCategories_;
    bool onlyEquippedPackMods_{false};
    QSet<QString> equippedModIds_;
};

/// QML-friendly List Model for individual asset files within one mod.
class AssetListModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum AssetRoles {
        RelativePathRole = Qt::UserRole + 1,
        CategoryRole,
        FileSizeRole,
        CriticalRole,
        ExclusiveSlotRole,
        EnabledRole,
        FormattedSizeRole
    };

    explicit AssetListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }

    void setAssets(const QString& modId, const QVector<AssetEntry>& assets);
    const QVector<AssetEntry>& assets() const { return assets_; }
    QString currentModId() const { return currentModId_; }

    int activeCount() const;
    int totalCount() const { return assets_.size(); }

    Q_INVOKABLE void toggleAsset(int row);

signals:
    void countChanged();
    void assetToggled(const QString& modId, const QString& relPath, bool enabled);
    void countsChanged(int active, int total);

private:
    QString currentModId_;
    QVector<AssetEntry> assets_;
};

/// QML-friendly List Model for Mod Pack presets.
class PresetListModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum PresetRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        NameEnRole,
        DescriptionEnRole,
        ModCountRole,
        UpdatedAtRole,
        IsEquippedRole
    };

    explicit PresetListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return rowCount(); }

    void setPresets(const QVector<PresetEntry>& presets);
    void setEquippedId(const QString& id);

    QString equippedId() const { return equippedId_; }

signals:
    void countChanged();

private:
    QVector<PresetEntry> presets_;
    QString equippedId_{"default"};
};

/// Central controller bridging QML UI with C++ SmmClient and local settings.
class GuiController : public QObject {
    Q_OBJECT

    Q_PROPERTY(ModListModel* modListModel READ modListModel CONSTANT)
    Q_PROPERTY(AssetListModel* assetListModel READ assetListModel CONSTANT)
    Q_PROPERTY(PresetListModel* presetListModel READ presetListModel CONSTANT)

    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(QVariantList availableFonts READ availableFonts NOTIFY availableFontsChanged)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY reducedMotionChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(QVariantList availableThemes READ availableThemes NOTIFY languageChanged)

    Q_PROPERTY(QString sekiroDir READ sekiroDir WRITE setSekiroDir NOTIFY sekiroDirChanged)
    Q_PROPERTY(QString stagingDir READ stagingDir WRITE setStagingDir NOTIFY stagingDirChanged)
    Q_PROPERTY(bool isNtfsMatched READ isNtfsMatched NOTIFY ntfsMatchedChanged)

    Q_PROPERTY(int deployedFiles READ deployedFiles NOTIFY telemetryChanged)
    Q_PROPERTY(quint64 bytesSaved READ bytesSaved NOTIFY telemetryChanged)
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY telemetryChanged)

    Q_PROPERTY(QString currentModId READ currentModId NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModName READ currentModName NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModAuthor READ currentModAuthor NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModVersion READ currentModVersion NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModDesc READ currentModDesc NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModPreview READ currentModPreview NOTIFY currentModChanged)
    Q_PROPERTY(int previewRevision READ previewRevision NOTIFY previewRevisionChanged)
    Q_PROPERTY(bool hasEquippedPack READ hasEquippedPack NOTIFY equippedPackChanged)
    Q_PROPERTY(QString equippedPackId READ equippedPackId NOTIFY equippedPackChanged)
    Q_PROPERTY(QString currentModCategory READ currentModCategory NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModSourceUrl READ currentModSourceUrl NOTIFY currentModChanged)
    Q_PROPERTY(QString currentModHomepage READ currentModHomepage NOTIFY currentModChanged)
    Q_PROPERTY(int activeAssetCount READ activeAssetCount NOTIFY assetCountsChanged)
    Q_PROPERTY(int totalAssetCount READ totalAssetCount NOTIFY assetCountsChanged)

    Q_PROPERTY(bool isDoctorChecking READ isDoctorChecking NOTIFY doctorChanged)
    Q_PROPERTY(bool launchPending READ launchPending NOTIFY doctorChanged)
    Q_PROPERTY(bool canLaunch READ canLaunch NOTIFY doctorChanged)
    Q_PROPERTY(QString healthSummary READ healthSummary NOTIFY doctorChanged)
    Q_PROPERTY(QString healthRemediation READ healthRemediation NOTIFY doctorChanged)
    Q_PROPERTY(bool engineReady READ engineReady NOTIFY doctorChanged)
    Q_PROPERTY(QString healthOverall READ healthOverall NOTIFY doctorChanged)
    Q_PROPERTY(int healthOkCount READ healthOkCount NOTIFY doctorChanged)
    Q_PROPERTY(int healthWarnCount READ healthWarnCount NOTIFY doctorChanged)
    Q_PROPERTY(int healthErrorCount READ healthErrorCount NOTIFY doctorChanged)
    Q_PROPERTY(QVariantList healthItems READ healthItems NOTIFY doctorChanged)
    Q_PROPERTY(UpdateManager* updater READ updater CONSTANT)

public:
    explicit GuiController(QObject* parent = nullptr);

    ModListModel* modListModel() { return &modListModel_; }
    AssetListModel* assetListModel() { return &assetListModel_; }
    PresetListModel* presetListModel() { return &presetListModel_; }
    UpdateManager* updater() { return &updater_; }

    bool isBusy() const { return client_.isBusy(); }
    QString statusText() const { return statusText_; }

    QString language() const { return language_; }
    void setLanguage(const QString& code);

    QString fontFamily() const { return fontFamily_; }
    void setFontFamily(const QString& family);

    QString themeMode() const { return themeMode_; }
    void setThemeMode(const QString& mode);

    QVariantList availableFonts() const;
    QVariantList availableThemes() const;
    static QString defaultFontFamily();

    bool reducedMotion() const { return reducedMotion_; }
    void setReducedMotion(bool reduced);

    static void applyFontFamily(const QString& family);
    static void applyThemeMode(const QString& mode);

    QString sekiroDir() const { return sekiroDir_; }
    void setSekiroDir(const QString& dir);

    QString stagingDir() const { return stagingDir_; }
    void setStagingDir(const QString& dir);

    bool isNtfsMatched() const;

    int deployedFiles() const { return deployedFiles_; }
    quint64 bytesSaved() const { return bytesSaved_; }
    int conflictCount() const { return conflictCount_; }

    QString currentModId() const { return currentModId_; }
    QString currentModName() const { return currentModName_; }
    QString currentModAuthor() const { return currentModAuthor_; }
    QString currentModVersion() const { return currentModVersion_; }
    QString currentModDesc() const { return currentModDesc_; }
    QString currentModPreview() const { return currentModPreview_; }
    int previewRevision() const { return previewRevision_; }
    bool hasEquippedPack() const { return !equippedPackId_.isEmpty(); }
    QString equippedPackId() const { return equippedPackId_; }
    QString currentModCategory() const { return currentModCategory_; }
    QString currentModSourceUrl() const { return currentModSourceUrl_; }
    QString currentModHomepage() const { return currentModHomepage_; }

    int activeAssetCount() const { return assetListModel_.activeCount(); }
    int totalAssetCount() const { return assetListModel_.totalCount(); }

    bool isDoctorChecking() const { return client_.isDoctorChecking(); }
    bool launchPending() const { return launchPending_ || deployLaunchPending_; }
    bool canLaunch() const { return !isBusy() && !isDoctorChecking() && modScanFailures_.isEmpty() &&
                                     healthInfo_.overall == QLatin1String("healthy"); }
    QString healthSummary() const;
    QString healthRemediation() const;
    bool engineReady() const;
    QString healthOverall() const { return healthInfo_.overall; }
    int healthOkCount() const { return healthInfo_.okCount; }
    int healthWarnCount() const { return healthInfo_.warningCount; }
    int healthErrorCount() const { return healthInfo_.errorCount; }
    QVariantList healthItems() const;

    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void refreshDoctor();
    Q_INVOKABLE void setupEngine();
    Q_INVOKABLE void deploy();
    Q_INVOKABLE void deployAndLaunch();
    Q_INVOKABLE void restore();
    Q_INVOKABLE void launchGame();
    Q_INVOKABLE void autoDetectGameDir();
    Q_INVOKABLE QString detectSekiroDir() const;
    Q_INVOKABLE void saveSettings(const QString& staging, const QString& game,
                                 const QString& lang = {}, const QString& font = {},
                                 const QString& theme = {}, bool reducedMotion = false);
    Q_INVOKABLE void openModDetail(const QString& modId);
    Q_INVOKABLE void setModPreview(const QString& modId, const QString& imagePath);
    Q_INVOKABLE void optimizeAllPreviews();
    Q_INVOKABLE void setModSourceUrl(const QString& modId, const QString& url);
    Q_INVOKABLE void updateModMetadata(const QString& modId, const QString& newId,
                                       const QString& name, const QString& author,
                                       const QString& version, const QString& category,
                                       const QString& description, const QString& sourceUrl);
    Q_INVOKABLE void updateModMetadata(const QString& modId, const QString& name,
                                       const QString& author, const QString& version,
                                       const QString& category, const QString& description,
                                       const QString& sourceUrl) {
        updateModMetadata(modId, modId, name, author, version, category, description, sourceUrl);
    }
    Q_INVOKABLE void deleteMod(const QString& modId);
    Q_INVOKABLE void deleteMods(const QStringList& modIds);
    Q_INVOKABLE void setModsEnabled(const QStringList& modIds, bool enabled);
    Q_INVOKABLE void exportSingleMod(const QString& modId, const QString& outputPath = {});
    Q_INVOKABLE void openUrl(const QString& url);
    Q_INVOKABLE void copyToClipboard(const QString& text);
    Q_INVOKABLE void openFolder(const QString& path);
    Q_INVOKABLE void openModFolder(const QString& modId);
    Q_INVOKABLE void openGameFolder();
    Q_INVOKABLE void openStagingFolder();
    Q_INVOKABLE void saveModPack(const QString& nameZh, const QString& descZh,
                                 const QString& nameEn = {}, const QString& descEn = {});
    Q_INVOKABLE void applyModPack(const QString& packId);
    Q_INVOKABLE void deactivateModPack();
    Q_INVOKABLE void deleteModPack(const QString& packId);
    Q_INVOKABLE void exportModPack(const QString& packId, const QString& outputPath);
    Q_INVOKABLE void importModPack(const QString& packPath);
    Q_INVOKABLE void importArchive(const QString& archivePath);
    Q_INVOKABLE void importArchives(const QStringList& archivePaths);

signals:
    void busyChanged();
    void statusTextChanged();
    void languageChanged(const QString& code);
    void fontFamilyChanged(const QString& family);
    void availableFontsChanged();
    void reducedMotionChanged();
    void themeModeChanged(const QString& mode);
    void sekiroDirChanged();
    void stagingDirChanged();
    void ntfsMatchedChanged();
    void telemetryChanged();
    void doctorChanged();
    void currentModChanged();
    void previewRevisionChanged();
    void equippedPackChanged();
    void assetCountsChanged();
    void notification(const QString& type, const QString& message);
    void launchBlocked();

protected:
    virtual bool startGameProcess(const QString& executable, const QString& workingDirectory);

private:
    void initSignals();
    void checkVolumeMatch();
    void completeLaunchCheck();
    void scheduleDoctor();
    bool launchPending_{false};
    bool deployLaunchPending_{false};
    bool doctorScheduled_{false};
    bool doctorRefreshPending_{false};

    SmmClient client_;
    ModListModel modListModel_;
    AssetListModel assetListModel_;
    PresetListModel presetListModel_;

    QString statusText_{"Ready"};
    QString language_{"zh-CN"};
    QString fontFamily_;
    mutable QVariantList availableFontsCache_;
    bool reducedMotion_{false};
    QString themeMode_{QStringLiteral("dark")};
    QString sekiroDir_;
    QString stagingDir_;
    bool isNtfsMatched_{false};

    int deployedFiles_{0};
    quint64 bytesSaved_{0};
    int conflictCount_{0};
    int previewRevision_{0};
    QString equippedPackId_;

    void updateEquippedModIds();
    void updateDeploymentTelemetry();

    QString currentModId_;
    QString currentModName_;
    QString currentModAuthor_;
    QString currentModVersion_;
    QString currentModDesc_;
    QString currentModPreview_;
    QString currentModCategory_;
    QString currentModSourceUrl_;
    QString currentModHomepage_;

    HealthInfo healthInfo_;
    QStringList modScanFailures_;
    UpdateManager updater_;
};

} // namespace smm::gui
