#include "Models.hpp"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>
#include <fstream>

#include "ModPreviewImageProvider.hpp"
#include <theme/hustheme.h>

#include <smm/doctor.hpp>
#include <smm/loader.hpp>
#include <smm/manager.hpp>
#include <smm/preset.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace smm::gui {

namespace {

QString formatBytes(quint64 bytes) {
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
    if (bytes >= 1024ULL * 1024ULL) {
        return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1) + " MB";
    }
    if (bytes >= 1024ULL) {
        return QString::number(static_cast<double>(bytes) / 1024.0, 'f', 0) + " KB";
    }
    return QString::number(bytes) + " B";
}

bool checkSameDrive(const QString& pathA, const QString& pathB) {
    if (pathA.isEmpty() || pathB.isEmpty()) return false;
#ifdef _WIN32
    WCHAR rootA[MAX_PATH] = {0};
    WCHAR rootB[MAX_PATH] = {0};
    if (GetVolumePathNameW(pathA.toStdWString().c_str(), rootA, MAX_PATH) &&
        GetVolumePathNameW(pathB.toStdWString().c_str(), rootB, MAX_PATH)) {
        DWORD serialA = 0, serialB = 0;
        if (GetVolumeInformationW(rootA, nullptr, 0, &serialA, nullptr, nullptr, nullptr, 0) &&
            GetVolumeInformationW(rootB, nullptr, 0, &serialB, nullptr, nullptr, nullptr, 0)) {
            return serialA == serialB && serialA != 0;
        }
    }
#endif
    return pathA.left(2).compare(pathB.left(2), Qt::CaseInsensitive) == 0;
}

} // namespace

// ============================================================================
// ModListModel Implementation
// ============================================================================

ModListModel::ModListModel(QObject* parent) : QAbstractListModel(parent) {}

int ModListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return visibleIndices_.size();
}

QVector<int> ModListModel::filteredIndices(const QVector<ModEntry>& mods) const {
    QVector<int> indices;
    indices.reserve(mods.size());
    const QString f = filterText_.trimmed();
    const QString cat = selectedCategory_.trimmed();
    for (int i = 0; i < mods.size(); ++i) {
        const auto& m = mods[i];
        if (onlyEquippedPackMods_) {
            if (!equippedModIds_.contains(m.id)) {
                continue;
            }
        }
        if (!cat.isEmpty() && cat != QLatin1String("all")) {
            if (m.category.compare(cat, Qt::CaseInsensitive) != 0) {
                continue;
            }
        }
        if (!f.isEmpty()) {
            if (!m.name.contains(f, Qt::CaseInsensitive) &&
                !m.author.contains(f, Qt::CaseInsensitive) &&
                !m.category.contains(f, Qt::CaseInsensitive) &&
                !m.id.contains(f, Qt::CaseInsensitive) &&
                !m.description.contains(f, Qt::CaseInsensitive)) {
                continue;
            }
        }
        indices.append(i);
    }
    return indices;
}

void ModListModel::rebuildFilter() {
    visibleIndices_ = filteredIndices(mods_);
}

void ModListModel::setSelectedCategory(const QString& cat) {
    if (selectedCategory_ == cat) return;
    beginResetModel();
    selectedCategory_ = cat;
    rebuildFilter();
    endResetModel();
    emit selectedCategoryChanged();
    emit countChanged();
}

void ModListModel::setOnlyEquippedPackMods(bool only) {
    if (onlyEquippedPackMods_ == only) return;
    beginResetModel();
    onlyEquippedPackMods_ = only;
    rebuildFilter();
    endResetModel();
    emit onlyEquippedPackModsChanged();
    emit countChanged();
}

void ModListModel::setEquippedModIds(const QSet<QString>& ids) {
    equippedModIds_ = ids;
    if (onlyEquippedPackMods_) {
        beginResetModel();
        rebuildFilter();
        endResetModel();
        emit countChanged();
    }
}

void ModListModel::setFilterText(const QString& filter) {
    if (filterText_ == filter) return;
    beginResetModel();
    filterText_ = filter;
    rebuildFilter();
    endResetModel();
    emit filterTextChanged();
    emit countChanged();
}

QVariant ModListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= visibleIndices_.size()) {
        return {};
    }
    const int realIndex = visibleIndices_.at(index.row());
    if (realIndex < 0 || realIndex >= mods_.size()) return {};
    const auto& m = mods_.at(realIndex);
    switch (role) {
        case IdRole: return m.id;
        case NameRole: return m.name;
        case VersionRole: return m.version;
        case AuthorRole: return m.author;
        case CategoryRole: return m.category;
        case DescriptionRole: return m.description;
        case HomepageRole: return m.homepage;
        case SourceUrlRole: return m.sourceUrl;
        case EnabledRole: return m.enabled;
        case PriorityRole: return m.priority;
        case AssetCountRole: return m.assetCount;
        case TotalBytesRole: return formatBytes(m.totalBytes);
        case RootPathRole: return m.rootPath;
        case PreviewImageRole: return m.previewImage;
        case PreviewImagePathRole: {
            if (!m.previewImage.isEmpty() && !m.rootPath.isEmpty()) {
                return QStringLiteral("image://modpreview/") + m.id;
            }
            return QString{};
        }
        case DisabledAssetsRole: return m.disabledAssets;
        case RankRole: return realIndex + 1;
        default: return {};
    }
}

QHash<int, QByteArray> ModListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {VersionRole, "version"},
        {AuthorRole, "author"},
        {CategoryRole, "category"},
        {DescriptionRole, "description"},
        {HomepageRole, "homepage"},
        {SourceUrlRole, "sourceUrl"},
        {EnabledRole, "enabled"},
        {PriorityRole, "priority"},
        {AssetCountRole, "assetCount"},
        {TotalBytesRole, "totalBytes"},
        {RootPathRole, "rootPath"},
        {PreviewImageRole, "previewImage"},
        {PreviewImagePathRole, "previewImagePath"},
        {DisabledAssetsRole, "disabledAssets"},
        {RankRole, "rank"}
    };
}

void ModListModel::setMods(const QVector<ModEntry>& mods) {
    QVector<ModEntry> sorted = mods;
    std::sort(sorted.begin(), sorted.end(), [](const ModEntry& a, const ModEntry& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.id < b.id;
    });
    const bool sameOrder = sorted.size() == mods_.size() &&
        std::equal(sorted.cbegin(), sorted.cend(), mods_.cbegin(), [](const ModEntry& a, const ModEntry& b) {
            return a.id == b.id;
        });
    // Metadata edits can change search/category membership even when IDs stay ordered.
    QVector<int> updatedIndices = filteredIndices(sorted);
    const bool resetRequired = !sameOrder || updatedIndices != visibleIndices_;
    if (resetRequired) beginResetModel();
    mods_ = std::move(sorted);
    visibleIndices_ = std::move(updatedIndices);

    QStringList cats;
    cats.append(QStringLiteral("all"));
    for (const auto& m : mods_) {
        if (!m.category.isEmpty() && !cats.contains(m.category)) {
            cats.append(m.category);
        }
    }
    const bool categoriesChanged = availableCategories_ != cats;
    availableCategories_ = cats;

    if (resetRequired) {
        endResetModel();
        emit countChanged();
    } else if (rowCount() > 0) {
        emit dataChanged(index(0), index(rowCount() - 1));
    }
    if (categoriesChanged) emit availableCategoriesChanged();
}

const ModEntry* ModListModel::findMod(const QString& id) const {
    for (const auto& m : mods_) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

void ModListModel::toggleEnabled(int row) {
    if (row < 0 || row >= visibleIndices_.size()) return;
    const int realIndex = visibleIndices_.at(row);
    if (realIndex < 0 || realIndex >= mods_.size()) return;
    mods_[realIndex].enabled = !mods_[realIndex].enabled;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {EnabledRole});
    emit modToggled(mods_[realIndex].id, mods_[realIndex].enabled);
}

void ModListModel::setPriority(int row, quint32 priority) {
    if (row < 0 || row >= visibleIndices_.size()) return;
    const int realIndex = visibleIndices_.at(row);
    if (realIndex < 0 || realIndex >= mods_.size()) return;
    mods_[realIndex].priority = priority;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {PriorityRole});
    emit priorityChanged(mods_[realIndex].id, priority);
}

void ModListModel::swapPriority(int rowA, int rowB) {
    if (rowA < 0 || rowA >= visibleIndices_.size() || rowB < 0 || rowB >= visibleIndices_.size() || rowA == rowB) return;
    const int realA = visibleIndices_.at(rowA);
    const int realB = visibleIndices_.at(rowB);
    if (realA < 0 || realA >= mods_.size() || realB < 0 || realB >= mods_.size()) return;

    const quint32 priA = mods_[realA].priority;
    const quint32 priB = mods_[realB].priority;

    mods_[realA].priority = priB;
    mods_[realB].priority = priA;

    beginResetModel();
    std::swap(mods_[realA], mods_[realB]);
    rebuildFilter();
    endResetModel();

    emit prioritiesSwapped(mods_[realB].id, priB, mods_[realA].id, priA);
}

void ModListModel::moveRow(int from, int to) {
    if (from < 0 || from >= visibleIndices_.size() || to < 0 || to >= visibleIndices_.size() || from == to) return;
    const int realFrom = visibleIndices_.at(from);
    const int realTo = visibleIndices_.at(to);
    if (realFrom < 0 || realFrom >= mods_.size() || realTo < 0 || realTo >= mods_.size()) return;

    if (!beginMoveRows({}, from, from, {}, to > from ? to + 1 : to)) return;
    mods_.move(realFrom, realTo);
    QMap<QString, quint32> batch;
    for (int i = 0; i < mods_.size(); ++i) {
        mods_[i].priority = static_cast<quint32>((i + 1) * 10);
        batch.insert(mods_[i].id, mods_[i].priority);
    }
    rebuildFilter();
    endMoveRows();
    emit dataChanged(index(0), index(rowCount() - 1), {PriorityRole, RankRole});

    emit batchPrioritiesChanged(batch);
}

// ============================================================================
// AssetListModel Implementation
// ============================================================================

AssetListModel::AssetListModel(QObject* parent) : QAbstractListModel(parent) {}

int AssetListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return assets_.size();
}

QVariant AssetListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= assets_.size()) return {};
    const auto& a = assets_.at(index.row());
    switch (role) {
        case RelativePathRole: return a.relativePath;
        case CategoryRole: return a.category;
        case FileSizeRole: return a.fileSize;
        case CriticalRole: return a.critical;
        case ExclusiveSlotRole: return a.exclusiveSlot;
        case EnabledRole: return a.enabled;
        case FormattedSizeRole: return formatBytes(a.fileSize);
        default: return {};
    }
}

QHash<int, QByteArray> AssetListModel::roleNames() const {
    return {
        {RelativePathRole, "relativePath"},
        {CategoryRole, "category"},
        {FileSizeRole, "fileSize"},
        {CriticalRole, "isCritical"},
        {ExclusiveSlotRole, "isExclusiveSlot"},
        {EnabledRole, "enabled"},
        {FormattedSizeRole, "formattedSize"}
    };
}

void AssetListModel::setAssets(const QString& modId, const QVector<AssetEntry>& assets) {
    beginResetModel();
    currentModId_ = modId;
    assets_ = assets;
    endResetModel();
    emit countChanged();
    emit countsChanged(activeCount(), totalCount());
}

int AssetListModel::activeCount() const {
    int count = 0;
    for (const auto& a : assets_) {
        if (a.enabled) ++count;
    }
    return count;
}

void AssetListModel::toggleAsset(int row) {
    if (row < 0 || row >= assets_.size()) return;
    assets_[row].enabled = !assets_[row].enabled;
    const QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx, {EnabledRole});
    emit assetToggled(currentModId_, assets_[row].relativePath, assets_[row].enabled);
    emit countsChanged(activeCount(), totalCount());
}

// ============================================================================
// PresetListModel Implementation
// ============================================================================

PresetListModel::PresetListModel(QObject* parent) : QAbstractListModel(parent) {}

int PresetListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return presets_.size();
}

QVariant PresetListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= presets_.size()) return {};
    const auto& p = presets_.at(index.row());
    switch (role) {
        case IdRole: return p.id;
        case NameRole: return p.name;
        case DescriptionRole: return p.description;
        case NameEnRole: return p.nameEn.isEmpty() ? p.name : p.nameEn;
        case DescriptionEnRole: return p.descriptionEn.isEmpty() ? p.description : p.descriptionEn;
        case ModCountRole: return p.modCount;
        case UpdatedAtRole: return p.updatedAt;
        case IsEquippedRole: return p.id == equippedId_;
        default: return {};
    }
}

QHash<int, QByteArray> PresetListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {DescriptionRole, "description"},
        {NameEnRole, "nameEn"},
        {DescriptionEnRole, "descriptionEn"},
        {ModCountRole, "modCount"},
        {UpdatedAtRole, "updatedAt"},
        {IsEquippedRole, "isEquipped"}
    };
}

void PresetListModel::setPresets(const QVector<PresetEntry>& presets) {
    beginResetModel();
    presets_ = presets;
    endResetModel();
    emit countChanged();
}

void PresetListModel::setEquippedId(const QString& id) {
    equippedId_ = id;
    beginResetModel();
    endResetModel();
}

// ============================================================================
// GuiController Implementation
// ============================================================================

GuiController::GuiController(QObject* parent) : QObject(parent) {
    initSignals();

    // 加载持久化设置
    QSettings settings("SekiroModManager", "SMM");
    stagingDir_ = settings.value("stagingDir").toString();
    sekiroDir_ = settings.value("sekiroDir").toString();
    language_ = settings.value("language", QStringLiteral("zh-CN")).toString();
    fontFamily_ = settings.value("fontFamily", defaultFontFamily()).toString().trimmed();
    if (fontFamily_.isEmpty()) fontFamily_ = defaultFontFamily();
    reducedMotion_ = settings.value("reducedMotion", false).toBool();
    HusTheme::instance()->setAnimationEnabled(!reducedMotion_);
    connect(qGuiApp, &QGuiApplication::fontDatabaseChanged, this, [this]() {
        availableFontsCache_.clear();
        emit availableFontsChanged();
    });

    themeMode_ = settings.value("themeMode", QStringLiteral("dark")).toString();
    if (themeMode_.trimmed().isEmpty()) themeMode_ = QStringLiteral("dark");

    equippedPackId_ = settings.value("equippedPresetId").toString();
    if (!equippedPackId_.isEmpty()) {
        presetListModel_.setEquippedId(equippedPackId_);
    }

    if (!stagingDir_.isEmpty()) client_.setStagingDir(stagingDir_);
    if (!sekiroDir_.isEmpty()) client_.setGameDir(sekiroDir_);

    applyFontFamily(fontFamily_);
    applyThemeMode(themeMode_);

    checkVolumeMatch();
    refreshAll();
    auto* runtimeTimer = new QTimer(this);
    runtimeTimer->setInterval(10000);
    connect(runtimeTimer, &QTimer::timeout, this, [this]() {
        if (!isBusy() && !launchPending()) client_.refreshRuntimeDoctor();
    });
    runtimeTimer->start();
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && !isBusy()) client_.refreshRuntimeDoctor();
    });
}

void GuiController::initSignals() {
    connect(&client_, &SmmClient::busyChanged, this, [this](bool busy, const QString& label) {
        statusText_ = busy ? label : "Ready";
        emit busyChanged();
        emit statusTextChanged();
        emit doctorChanged();
        if (!busy && doctorRefreshPending_ && !client_.isDoctorChecking()) {
            doctorRefreshPending_ = false;
            scheduleDoctor();
        }
    });

    connect(&client_, &SmmClient::environmentLoaded, this, [this](const EnvironmentInfo& env) {
        stagingDir_ = env.stagingDir;
        sekiroDir_ = env.gameDir;
        checkVolumeMatch();
        emit stagingDirChanged();
        emit sekiroDirChanged();
    });

    connect(&client_, &SmmClient::modsLoaded, this, [this](const QVector<ModEntry>& mods, const QStringList& failures, const QString&) {
        modListModel_.setMods(mods);
        modScanFailures_ = failures;
        emit doctorChanged();
        scheduleDoctor();
    });

    connect(&client_, &SmmClient::modDetailLoaded, this, [this](const ModEntry& mod, const QVector<AssetEntry>& assets) {
        currentModId_ = mod.id;
        currentModName_ = mod.name;
        currentModAuthor_ = mod.author;
        currentModVersion_ = mod.version;
        currentModDesc_ = mod.description;
        currentModPreview_ = mod.previewImage;
        currentModCategory_ = mod.category;
        currentModSourceUrl_ = mod.sourceUrl;
        currentModHomepage_ = mod.homepage;
        assetListModel_.setAssets(mod.id, assets);
        emit currentModChanged();
        emit assetCountsChanged();
    });

    connect(&client_, &SmmClient::presetsLoaded, this, [this](const QVector<PresetEntry>& presets) {
        presetListModel_.setPresets(presets);
        if (!equippedPackId_.isEmpty()) {
            presetListModel_.setEquippedId(equippedPackId_);
            updateEquippedModIds();
        }
    });

    connect(&client_, &SmmClient::conflictsLoaded, this, [this](const QVector<ConflictEntry>& conflicts) {
        conflictCount_ = conflicts.size();
        emit telemetryChanged();
    });

    connect(&client_, &SmmClient::doctorLoaded, this, [this](const HealthInfo& health) {
        healthInfo_ = health;
        emit doctorChanged();
    });

    connect(&client_, &SmmClient::doctorCheckingChanged, this, [this]() {
        emit doctorChanged();
        if (client_.isDoctorChecking()) return;
        if (doctorRefreshPending_ && !isBusy()) {
            doctorRefreshPending_ = false;
            refreshDoctor();
            return;
        }
        if (launchPending_) completeLaunchCheck();
    });

    connect(&client_, &SmmClient::deployFinished, this, [this](const DeploySummary& summary, const PlanInfo&) {
        const bool launchAfter = deployLaunchPending_;
        deployLaunchPending_ = false;
        updateDeploymentTelemetry();
        if (!summary.success || summary.failed > 0) {
            emit notification("error", tr("部署失败：%1 个文件未部署，已取消启动。%2")
                              .arg(summary.failed).arg(summary.errors.join(QStringLiteral("; "))));
            emit launchBlocked();
            refreshDoctor();
            return;
        }
        emit notification("success", tr("Deployment succeeded: %1 files linked.").arg(summary.hardLinks + summary.copies));
        if (launchAfter) launchGame();
        else refreshDoctor();
    });

    connect(&client_, &SmmClient::restoreFinished, this, [this](const RestoreSummary& summary) {
        deployedFiles_ = 0;
        bytesSaved_ = 0;
        emit telemetryChanged();
        emit notification("info", tr("Restored: %1 links safely unlinked.").arg(summary.removedFiles));
        refreshDoctor();
    });

    connect(&client_, &SmmClient::operationSucceeded, this, [this](const QString& title, const QString& detail) {
        emit notification("success", QString("%1: %2").arg(title, detail));
        // Refresh checks after engine/config mutations as well as mod model updates.
        scheduleDoctor();
    });

    connect(&client_, &SmmClient::operationFailed, this, [this](const QString& title, const QString& message) {
        if (deployLaunchPending_) {
            deployLaunchPending_ = false;
            emit doctorChanged();
            emit launchBlocked();
        }
        emit notification("error", QString("%1: %2").arg(title, message));
        scheduleDoctor();
    });

    // 联动 ModListModel 开关与优先级
    connect(&modListModel_, &ModListModel::modToggled, &client_, &SmmClient::setModEnabled);
    connect(&modListModel_, &ModListModel::priorityChanged, &client_, &SmmClient::setModPriority);
    connect(&modListModel_, &ModListModel::prioritiesSwapped, &client_, &SmmClient::swapModPriorities);
    connect(&modListModel_, &ModListModel::batchPrioritiesChanged, &client_, &SmmClient::batchUpdatePriorities);

    // 联动 AssetListModel 资产开关
    connect(&assetListModel_, &AssetListModel::assetToggled, &client_, &SmmClient::setAssetEnabled);

    // 联动 UpdateManager 弹窗通知
    connect(&updater_, &UpdateManager::updateNotification, this, &GuiController::notification);
}

void GuiController::checkVolumeMatch() {
    isNtfsMatched_ = checkSameDrive(stagingDir_, sekiroDir_);
    emit ntfsMatchedChanged();
}

void GuiController::setSekiroDir(const QString& dir) {
    if (sekiroDir_ != dir) {
        sekiroDir_ = dir;
        client_.setGameDir(dir);
        checkVolumeMatch();
        updateDeploymentTelemetry();
        emit sekiroDirChanged();
        launchPending_ = false;
        deployLaunchPending_ = false;
        refreshDoctor();
    }
}

void GuiController::setStagingDir(const QString& dir) {
    if (stagingDir_ != dir) {
        stagingDir_ = dir;
        client_.setStagingDir(dir);
        checkVolumeMatch();
        emit stagingDirChanged();
        launchPending_ = false;
        deployLaunchPending_ = false;
        refreshDoctor();
    }
}

bool GuiController::isNtfsMatched() const {
    return isNtfsMatched_;
}

void GuiController::setLanguage(const QString& code) {
    if (language_ == code) {
        return;
    }

    language_ = code;
    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("language", language_);
    availableFontsCache_.clear();
    emit languageChanged(language_);
    emit availableFontsChanged();
    emit doctorChanged();
}

void GuiController::setFontFamily(const QString& family) {
    const QString cleanFamily = family.trimmed();
    if (cleanFamily.isEmpty() || fontFamily_ == cleanFamily) {
        return;
    }
    fontFamily_ = cleanFamily;
    applyFontFamily(fontFamily_);
    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("fontFamily", fontFamily_);
    emit fontFamilyChanged(fontFamily_);
}

void GuiController::setThemeMode(const QString& mode) {
    if (themeMode_ == mode) {
        return;
    }
    themeMode_ = mode;
    applyThemeMode(themeMode_);
    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("themeMode", themeMode_);
    emit themeModeChanged(themeMode_);
}

QString GuiController::defaultFontFamily() {
    const QStringList installed = QFontDatabase::families();
    const QStringList preferred = {QStringLiteral("Noto Sans SC"), QStringLiteral("Source Han Sans SC"),
                                   QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")};
    for (const QString& family : preferred) {
        if (installed.contains(family, Qt::CaseInsensitive)) return family;
    }
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

void GuiController::setReducedMotion(bool reduced) {
    if (reducedMotion_ == reduced) return;
    reducedMotion_ = reduced;
    HusTheme::instance()->setAnimationEnabled(!reducedMotion_);
    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("reducedMotion", reducedMotion_);
    emit reducedMotionChanged();
}

void GuiController::applyFontFamily(const QString& family) {
    const QString targetFamily = family.trimmed().isEmpty() ? defaultFontFamily() : family.trimmed();
    QStringList fontFamilies = {targetFamily, QStringLiteral("Noto Sans SC"),
                               QStringLiteral("Source Han Sans SC"), QStringLiteral("Microsoft YaHei UI"),
                               QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei")};
    fontFamilies.removeDuplicates();
    QFont defaultFont = QGuiApplication::font();
    defaultFont.setStyleHint(QFont::SansSerif);
    defaultFont.setWeight(QFont::Normal);
    defaultFont.setFamilies(fontFamilies);
    QGuiApplication::setFont(defaultFont);

    // HuskarUI resolves this family list against the installed font database.
    // Keep the user's exact selection first, including Segoe UI.
    QStringList themeFamilies;
    for (const QString& fontFamily : fontFamilies) {
        themeFamilies.append(fontFamily);
    }
    themeFamilies.append(QStringLiteral("sans-serif"));
    HusTheme::instance()->installThemePrimaryFontFamiliesBase(themeFamilies.join(QStringLiteral(", ")));
    HusTheme::instance()->installThemePrimaryFontSizeBase(14);
}

void GuiController::applyThemeMode(const QString& mode) {
    const QString m = mode.trimmed().toLower();
    if (m == QLatin1String("light")) {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::Light);
    } else if (m == QLatin1String("system")) {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::System);
    } else {
        HusTheme::instance()->setDarkMode(HusTheme::DarkMode::Dark);
    }
}

QVariantList GuiController::availableFonts() const {
    if (!availableFontsCache_.isEmpty()) return availableFontsCache_;

    QStringList installed = QFontDatabase::families();
    installed.removeDuplicates();
    const QStringList preferred = {QStringLiteral("Noto Sans SC"), QStringLiteral("Source Han Sans SC"),
                                   QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Segoe UI")};
    QStringList ordered;
    for (const QString& preferredFamily : preferred) {
        const auto it = std::find_if(installed.cbegin(), installed.cend(), [&preferredFamily](const QString& family) {
            return family.compare(preferredFamily, Qt::CaseInsensitive) == 0;
        });
        if (it != installed.cend()) ordered.append(*it);
    }
    for (const QString& family : installed) {
        if (!ordered.contains(family)) ordered.append(family);
    }

    const bool isEn = language_ == QLatin1String("en-US");
    for (const QString& family : ordered) {
        if (family.startsWith(QLatin1Char('@')) || QFontDatabase::isPrivateFamily(family) ||
            family.startsWith(QLatin1String("HuskarUI"), Qt::CaseInsensitive)) continue;
        const bool recommended = family.compare(QLatin1String("Noto Sans SC"), Qt::CaseInsensitive) == 0;
        QString label = family;
        if (recommended) label += isEn ? QStringLiteral(" (Recommended)") : QStringLiteral("（思源黑体系列 · 推荐）");
        else if (family == QLatin1String("Source Han Sans SC"))
            label += isEn ? QStringLiteral(" (Source Han Sans)") : QStringLiteral("（思源黑体）");
        else if (!isEn && family == QLatin1String("Microsoft YaHei UI")) label += QStringLiteral("（微软雅黑 UI）");
        else if (!isEn && family == QLatin1String("Microsoft YaHei")) label += QStringLiteral("（微软雅黑）");
        availableFontsCache_.append(QVariantMap{{QStringLiteral("label"), label},
                                               {QStringLiteral("value"), family}});
    }
    return availableFontsCache_;
}

QVariantList GuiController::availableThemes() const {
    const bool isEn = (language_ == QLatin1String("en-US"));
    QVariantList result;
    {
        QVariantMap opt;
        opt[QStringLiteral("label")] = isEn ? QStringLiteral("Dark Theme (Default)") : QStringLiteral("暗色主题 (默认)");
        opt[QStringLiteral("value")] = QStringLiteral("dark");
        result.append(opt);
    }
    {
        QVariantMap opt;
        opt[QStringLiteral("label")] = isEn ? QStringLiteral("Light Theme") : QStringLiteral("亮色主题");
        opt[QStringLiteral("value")] = QStringLiteral("light");
        result.append(opt);
    }
    {
        QVariantMap opt;
        opt[QStringLiteral("label")] = isEn ? QStringLiteral("Follow System") : QStringLiteral("跟随系统");
        opt[QStringLiteral("value")] = QStringLiteral("system");
        result.append(opt);
    }
    return result;
}

void GuiController::refreshAll() {
    client_.refreshEnvironment();
    client_.refreshModState();
    client_.refreshPresets();
    scheduleDoctor();
    updateDeploymentTelemetry();
}

void GuiController::deploy() {
    if (isBusy() || launchPending()) return;
    client_.deploy();
}

void GuiController::deployAndLaunch() {
    if (isBusy() || launchPending()) return;
    deployLaunchPending_ = true;
    launchPending_ = true;
    emit doctorChanged();
    refreshDoctor();
}

void GuiController::restore() {
    if (isBusy() || launchPending()) return;
    client_.restore();
}

void GuiController::scheduleDoctor() {
    if (isBusy() || client_.isDoctorChecking()) {
        doctorRefreshPending_ = true;
        return;
    }
    doctorRefreshPending_ = false;
    if (doctorScheduled_) return;
    doctorScheduled_ = true;
    QTimer::singleShot(0, this, [this]() {
        doctorScheduled_ = false;
        if (isBusy()) {
            doctorRefreshPending_ = true;
            return;
        }
        if (!client_.isDoctorChecking()) refreshDoctor();
    });
}

void GuiController::launchGame() {
    if (isBusy() || launchPending()) return;
    launchPending_ = true;
    emit doctorChanged();
    // Always request a fresh complete check; cached green status never authorizes launch.
    refreshDoctor();
}

void GuiController::completeLaunchCheck() {
    launchPending_ = false;
    emit doctorChanged();
    if (!canLaunch() || QDir::cleanPath(QDir::fromNativeSeparators(healthInfo_.gameDir)) != QDir::cleanPath(QDir::fromNativeSeparators(sekiroDir_)) ||
        QDir::cleanPath(QDir::fromNativeSeparators(healthInfo_.stagingDir)) != QDir::cleanPath(QDir::fromNativeSeparators(stagingDir_))) {
        deployLaunchPending_ = false;
        emit doctorChanged();
        emit notification("error", tr("启动已阻止：%1").arg(healthSummary()));
        emit launchBlocked();
        return;
    }
    for (const auto& item : healthInfo_.items) {
        if (item.id.startsWith(QLatin1String("runtime-game-"))) {
            deployLaunchPending_ = false;
            emit doctorChanged();
            emit notification("info", tr("只狼已在运行，请先退出游戏再启动。"));
            emit launchBlocked();
            return;
        }
    }
    if (deployLaunchPending_) {
        client_.deploy();
        return;
    }
    const QString executable = QDir(sekiroDir_).filePath(QStringLiteral("sekiro.exe"));
    if (!QFileInfo(executable).isFile() || !startGameProcess(executable, sekiroDir_)) {
        emit notification("error", tr("启动失败，请检查游戏目录与访问权限。"));
        emit launchBlocked();
        return;
    }
    emit notification("success", tr("启动前诊断通过，已启动只狼。"));
    // Steam may relaunch the executable. Re-observe the actual process instead of
    // assuming a successful CreateProcess means that ModEngine loaded.
    QTimer::singleShot(3000, this, [this]() { client_.refreshRuntimeDoctor(); });
    QTimer::singleShot(10000, this, [this]() { client_.refreshRuntimeDoctor(); });
}

bool GuiController::startGameProcess(const QString& executable, const QString& workingDirectory) {
    return QProcess::startDetached(executable, QStringList{}, workingDirectory);
}

void GuiController::updateDeploymentTelemetry() {
    deployedFiles_ = 0;
    bytesSaved_ = 0;

    if (!sekiroDir_.isEmpty()) {
        fs::path modsDir = smm::utf8_to_path(sekiroDir_.toStdString());
        if (modsDir.filename() != "mods") {
            modsDir /= "mods";
        }
        const fs::path manifestPath = modsDir / ".smm_manifest.json";
        std::error_code ec;
        if (fs::exists(manifestPath, ec)) {
            try {
                std::ifstream f(manifestPath, std::ios::binary);
                if (f) {
                    nlohmann::json j;
                    f >> j;
                    if (j.contains("files") && j["files"].is_array()) {
                        deployedFiles_ = static_cast<int>(j["files"].size());
                    }
                    if (j.contains("total_bytes") && j["total_bytes"].is_number()) {
                        bytesSaved_ = j["total_bytes"].get<quint64>();
                    }
                }
            } catch (...) {
            }
        }
    }
    emit telemetryChanged();
}

QString GuiController::detectSekiroDir() const {
    const auto detected = smm::detect_game_dir();
    if (!detected.empty()) {
        return QString::fromStdWString(detected.wstring());
    }
    return {};
}

void GuiController::autoDetectGameDir() {
    const QString detected = detectSekiroDir();
    if (!detected.isEmpty()) {
        setSekiroDir(detected);
        emit notification("success", tr("Auto-detected Sekiro installation: %1").arg(detected));
        return;
    }
    emit notification("warning", tr("Could not auto-detect Sekiro in default Steam libraries. Please select manually."));
}

void GuiController::saveSettings(const QString& staging, const QString& game,
                                const QString& lang, const QString& font,
                                const QString& theme, bool reducedMotion) {
    const QString cleanStaging = staging.trimmed();
    const QString cleanGame = game.trimmed();
    const QString cleanLang = lang.trimmed();
    const QString cleanFont = font.trimmed();
    const QString cleanTheme = theme.trimmed();
    const bool pathsChanged = cleanStaging != stagingDir_ || cleanGame != sekiroDir_;

    setReducedMotion(reducedMotion);
    setStagingDir(cleanStaging);
    setSekiroDir(cleanGame);
    if (!cleanLang.isEmpty()) {
        setLanguage(cleanLang);
    }
    if (!cleanFont.isEmpty()) {
        setFontFamily(cleanFont);
    }
    if (!cleanTheme.isEmpty()) {
        setThemeMode(cleanTheme);
    }

    if (pathsChanged) client_.saveConfig(cleanStaging, cleanGame);

    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("stagingDir", cleanStaging);
    settings.setValue("sekiroDir", cleanGame);
    if (!cleanLang.isEmpty()) {
        settings.setValue("language", cleanLang);
    }
    if (!cleanFont.isEmpty()) {
        settings.setValue("fontFamily", cleanFont);
    }
    if (!cleanTheme.isEmpty()) {
        settings.setValue("themeMode", cleanTheme);
    }
    settings.sync();

    if (pathsChanged) refreshAll();

    emit notification("success", tr("Preferences saved successfully."));
}

void GuiController::openModDetail(const QString& modId) {
    client_.refreshModDetail(modId);
}

void GuiController::setModPreview(const QString& modId, const QString& imagePath) {
    QString cleanPath = imagePath.trimmed();
    if (cleanPath.startsWith(QStringLiteral("file:///"))) {
        cleanPath = cleanPath.mid(8);
    } else if (cleanPath.startsWith(QStringLiteral("file://"))) {
        cleanPath = cleanPath.mid(7);
    }
    cleanPath = QUrl::fromPercentEncoding(cleanPath.toUtf8());
    cleanPath = QDir::toNativeSeparators(cleanPath);

    client_.setModPreview(modId, cleanPath);
    ModPreviewImageProvider::clearCache();
    previewRevision_++;
    emit previewRevisionChanged();
    // The client has already refreshed mod state and detail.
}

void GuiController::optimizeAllPreviews() {
    if (stagingDir_.isEmpty()) return;
    const std::filesystem::path stagingPath = client_.stagingDir().toStdWString();

    QThreadPool::globalInstance()->start([this, stagingPath]() {
        const auto [count, saved] = smm::ModManager::optimize_all_previews(stagingPath);
        ModPreviewImageProvider::clearCache();
        QMetaObject::invokeMethod(this, [this, count, saved]() {
            previewRevision_++;
            emit previewRevisionChanged();
            refreshAll();

            const double savedMb = static_cast<double>(saved) / (1024.0 * 1024.0);
            emit notification(QStringLiteral("success"),
                              tr("背景图压缩优化完成：共压缩 %1 个模组背景图，释放了 %2 MB 空间！")
                              .arg(count).arg(QString::number(savedMb, 'f', 1)));
        });
    });
}

void GuiController::saveModPack(const QString& nameZh, const QString& descZh,
                                const QString& nameEn, const QString& descEn) {
    client_.savePreset(nameZh, descZh, nameEn, descEn);
}

void GuiController::deleteMods(const QStringList& modIds) {
    if (modIds.isEmpty()) return;
    client_.removeMods(modIds);
}

void GuiController::setModsEnabled(const QStringList& modIds, bool enabled) {
    if (modIds.isEmpty()) return;
    client_.setModsEnabled(modIds, enabled);
}

void GuiController::applyModPack(const QString& packId) {
    presetListModel_.setEquippedId(packId);
    equippedPackId_ = packId;
    updateEquippedModIds();
    emit equippedPackChanged();

    QSettings settings(QStringLiteral("SekiroModManager"), QStringLiteral("SMM"));
    settings.setValue(QStringLiteral("equippedPresetId"), packId);

    client_.applyPreset(packId);
}

void GuiController::deactivateModPack() {
    presetListModel_.setEquippedId({});
    equippedPackId_.clear();
    updateEquippedModIds();
    emit equippedPackChanged();

    QSettings settings(QStringLiteral("SekiroModManager"), QStringLiteral("SMM"));
    settings.remove(QStringLiteral("equippedPresetId"));

    emit notification(QStringLiteral("info"), tr("整合包已取消应用，当前进入自由模组管理状态。"));
}

void GuiController::updateEquippedModIds() {
    QSet<QString> ids;
    if (!equippedPackId_.isEmpty()) {
        const std::filesystem::path stagingPath = client_.stagingDir().toStdWString();
        std::error_code ec;
        if (std::filesystem::exists(stagingPath, ec)) {
            const auto presets = smm::PresetManager::list_presets(stagingPath);
            for (const auto& p : presets) {
                if (QString::fromStdString(p.id) == equippedPackId_) {
                    for (const auto& m : p.mods) {
                        ids.insert(QString::fromStdString(m.mod_id));
                    }
                    break;
                }
            }
        }
    }
    modListModel_.setEquippedModIds(ids);
}

void GuiController::exportModPack(const QString& packId, const QString& outputPath) {
    const std::filesystem::path stagingPath = client_.stagingDir().toStdWString();
    auto presets = smm::PresetManager::list_presets(stagingPath);
    QString packName = QStringLiteral("SekiroModPack");
    QString packDesc = QStringLiteral("Custom Modpack exported from SMM");
    QStringList ids;

    for (const auto& p : presets) {
        if (QString::fromStdString(p.id) == packId) {
            packName = QString::fromStdString(p.name);
            if (p.description) packDesc = QString::fromStdString(*p.description);
            for (const auto& m : p.mods) {
                ids.append(QString::fromStdString(m.mod_id));
            }
            break;
        }
    }

    if (ids.isEmpty()) {
        for (const auto& m : modListModel_.mods()) {
            if (m.enabled) ids.append(m.id);
        }
    }

    client_.exportModPack(ids, outputPath, packName, packDesc);
}

void GuiController::importModPack(const QString& packPath) {
    client_.importModPack(packPath);
}

void GuiController::importArchive(const QString& archivePath) {
    importArchives({archivePath});
}

void GuiController::importArchives(const QStringList& archivePaths) {
    QStringList cleanPaths;
    for (const auto& raw : archivePaths) {
        QString s = raw.trimmed();
        if (s.startsWith(QStringLiteral("file:///"))) {
            s = s.mid(8);
        } else if (s.startsWith(QStringLiteral("file://"))) {
            s = s.mid(7);
        }
        s = QUrl::fromPercentEncoding(s.toUtf8());
        s = QDir::toNativeSeparators(s);
        if (!s.isEmpty()) {
            cleanPaths.append(s);
        }
    }
    if (!cleanPaths.isEmpty()) {
        client_.importPaths(cleanPaths, {}, {}, true);
    }
}

QString GuiController::healthSummary() const {
    if (isDoctorChecking()) return tr("正在检查环境、部署文件与启动链…");
    const auto items = healthItems();
    for (const auto& value : items) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("id")).toString().startsWith(QLatin1String("mod-load-failure-")))
            return item.value(QStringLiteral("detail")).toString();
    }
    if (healthInfo_.overall.isEmpty()) return tr("尚未完成诊断，启动前将重新检测。");
    if (healthInfo_.overall == QLatin1String("healthy") && modScanFailures_.isEmpty())
        return tr("诊断通过，启动前仍会重新检查。");
    for (const auto& value : items) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("status")) == QLatin1String("error") ||
            item.value(QStringLiteral("status")) == QLatin1String("warning"))
            return item.value(QStringLiteral("detail")).toString();
    }
    return tr("诊断异常，请处理问题后重新检测。");
}

QString GuiController::healthRemediation() const {
    if (isDoctorChecking()) return {};
    for (const auto& value : healthItems()) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("id")).toString().startsWith(QLatin1String("mod-load-failure-")))
            return item.value(QStringLiteral("remediation")).toString();
    }
    for (const auto& value : healthItems()) {
        const auto item = value.toMap();
        if (item.value(QStringLiteral("status")) == QLatin1String("error") ||
            item.value(QStringLiteral("status")) == QLatin1String("warning"))
            return item.value(QStringLiteral("remediation")).toString();
    }
    return {};
}

bool GuiController::engineReady() const {
    if (isDoctorChecking() || healthInfo_.overall.isEmpty()) return false;
    bool hookPresent = false;
    for (const auto& item : healthInfo_.items) {
        if (item.id == QLatin1String("dinput8") && item.status == QLatin1String("ok")) hookPresent = true;
        if ((item.category == QLatin1String("modengine") || item.category == QLatin1String("runtime")) &&
            (item.status == QLatin1String("error") || item.status == QLatin1String("warning"))) return false;
    }
    return hookPresent;
}

QVariantList GuiController::healthItems() const {
    QVariantList list;
    for (const auto& item : healthInfo_.items) {
        QVariantMap m;
        m[QStringLiteral("id")] = item.id;
        m[QStringLiteral("category")] = item.category;
        m[QStringLiteral("title")] = item.title;
        m[QStringLiteral("detail")] = item.detail;
        m[QStringLiteral("status")] = item.status;
        m[QStringLiteral("remediation")] = item.remediation;
        if (language_ != QLatin1String("en-US") && item.category == QLatin1String("runtime")) {
            const bool hook = item.id.endsWith(QLatin1String("-hook"));
            const QString process = item.id.startsWith(QLatin1String("runtime-launcher-")) ? QStringLiteral("SMM") :
                item.id.startsWith(QLatin1String("runtime-steam-")) ? QStringLiteral("Steam") : QStringLiteral("Sekiro");
            m[QStringLiteral("title")] = hook ? tr("运行中的 ModEngine 钩子") : tr("%1 DLL 加载策略").arg(process);
            if (item.detail.contains(QLatin1String("PreferSystem32 is enabled"))) {
                m[QStringLiteral("detail")] = tr("%1 优先加载系统 DLL（PreferSystem32），可能绕过游戏目录中的 ModEngine，导致所有模组失效。")
                    .arg(process);
                m[QStringLiteral("remediation")] = tr("完全退出游戏和 Steam，从不带该策略的启动入口重新启动 Steam 与 SMM，再次检测；无需关闭全局 Windows 防护。");
            } else if (hook && item.status == QLatin1String("error")) {
                m[QStringLiteral("detail")] = item.detail.contains(QLatin1String("not loaded")) ?
                    tr("游戏未加载本地 ModEngine 钩子。文件已部署不代表模组已生效。") :
                    tr("无法核验游戏加载的 DLL，请先退出游戏并确认安装路径。") + QStringLiteral(" ") + item.detail;
                m[QStringLiteral("remediation")] = tr("完全退出游戏和 Steam，检查启动链后重新启动并再次检测。");
            } else if (item.status == QLatin1String("ok")) {
                m[QStringLiteral("detail")] = hook ? tr("游戏已加载本地 ModEngine 钩子；这不代表每个模组都已验证。") :
                    tr("%1 未启用优先加载系统 DLL 策略。").arg(process);
            } else {
                m[QStringLiteral("detail")] = tr("无法核验启动链，不能视为诊断通过。") + QStringLiteral(" ") + item.detail;
                m[QStringLiteral("remediation")] = tr("等待启动器完成启动，确认可以访问目标进程后重新检测。");
            }
        } else if (language_ != QLatin1String("en-US") && item.id == QLatin1String("deployment")) {
            m[QStringLiteral("title")] = tr("部署文件一致性");
            m[QStringLiteral("detail")] = item.status == QLatin1String("ok") ?
                tr("当前启用文件与部署结果一致，或没有需要部署的文件。") :
                tr("部署文件缺失、改变或已过期。") + QStringLiteral(" ") + item.detail;
            m[QStringLiteral("remediation")] = item.status == QLatin1String("ok") ? QString{} :
                tr("先部署当前模组组合，再重新检测。");
        }
        list.append(m);
    }
    for (qsizetype i = 0; i < modScanFailures_.size(); ++i) {
        list.append(QVariantMap{
            {QStringLiteral("id"), QStringLiteral("mod-load-failure-%1").arg(i)},
            {QStringLiteral("category"), tr("模组")},
            {QStringLiteral("title"), tr("模组读取失败")},
            {QStringLiteral("detail"), modScanFailures_.at(i)},
            {QStringLiteral("remediation"), tr("请重新导入该模组，或修复其元数据和文件后重新检测。")},
            {QStringLiteral("status"), QStringLiteral("error")}});
    }
    return list;
}

void GuiController::refreshDoctor() {
    // Refresh the exact mod failure list as well as the health report so successful repairs
    // can clear the launch gate without requiring an application restart.
    doctorRefreshPending_ = false;
    client_.refreshModState();
    doctorRefreshPending_ = false;
    client_.refreshDoctor();
}

void GuiController::setupEngine() {
    client_.setupEngine();
}

void GuiController::deleteMod(const QString& modId) {
    if (!modId.isEmpty()) {
        client_.removeMod(modId);
    }
}

void GuiController::exportSingleMod(const QString& modId, const QString& outputPath) {
    if (!modId.isEmpty()) {
        client_.exportSingleMod(modId, outputPath);
    }
}

void GuiController::updateModMetadata(const QString& modId, const QString& newId,
                                      const QString& name, const QString& author,
                                      const QString& version, const QString& category,
                                      const QString& description, const QString& sourceUrl) {
    client_.updateModMetadata(modId, newId, name, author, version, category, description, sourceUrl);
}

void GuiController::deleteModPack(const QString& packId) {
    if (!packId.isEmpty()) {
        client_.deletePreset(packId);
    }
}

void GuiController::openFolder(const QString& path) {
    if (!path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void GuiController::openModFolder(const QString& modId) {
    if (!stagingDir_.isEmpty() && !modId.isEmpty()) {
        const QString p = QDir(stagingDir_).filePath(modId);
        QDesktopServices::openUrl(QUrl::fromLocalFile(p));
    }
}

void GuiController::openGameFolder() {
    openFolder(sekiroDir_);
}

void GuiController::openStagingFolder() {
    openFolder(stagingDir_);
}

void GuiController::openUrl(const QString& url) {
    if (!url.trimmed().isEmpty()) {
        QDesktopServices::openUrl(QUrl(url.trimmed()));
    }
}

void GuiController::copyToClipboard(const QString& text) {
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        emit notification("success", tr("Copied to clipboard."));
    }
}

void GuiController::setModSourceUrl(const QString& modId, const QString& url) {
    if (modId.isEmpty()) return;
    try {
        std::filesystem::path stagingPath = stagingDir_.toStdWString();
        auto modDir = smm::ModManager::find_mod_dir(stagingPath, modId.toStdString());
        auto info = smm::ModLoader::load_mod_info(modDir);
        info.source_url = url.toStdString();
        smm::ModManager::save_mod_info(modDir, info);
        currentModSourceUrl_ = url;
        emit currentModChanged();
        emit notification("success", tr("Source URL updated."));
        refreshAll();
    } catch (const std::exception& e) {
        emit notification("error", QString::fromUtf8(e.what()));
    }
}

} // namespace smm::gui
