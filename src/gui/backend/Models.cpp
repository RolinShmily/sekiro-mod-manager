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
}

void GuiController::initSignals() {
    connect(&client_, &SmmClient::busyChanged, this, [this](bool busy, const QString& label) {
        statusText_ = busy ? label : "Ready";
        emit busyChanged();
        emit statusTextChanged();
    });

    connect(&client_, &SmmClient::environmentLoaded, this, [this](const EnvironmentInfo& env) {
        stagingDir_ = env.stagingDir;
        sekiroDir_ = env.gameDir;
        checkVolumeMatch();
        emit stagingDirChanged();
        emit sekiroDirChanged();
    });

    connect(&client_, &SmmClient::modsLoaded, this, [this](const QVector<ModEntry>& mods, const QStringList&, const QString&) {
        modListModel_.setMods(mods);
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

    connect(&client_, &SmmClient::deployFinished, this, [this](const DeploySummary& summary, const PlanInfo&) {
        deployedFiles_ = summary.hardLinks + summary.copies;
        bytesSaved_ = summary.bytesSaved;
        emit telemetryChanged();
        emit notification("success", tr("Deployment succeeded: %1 files linked.").arg(deployedFiles_));
    });

    connect(&client_, &SmmClient::restoreFinished, this, [this](const RestoreSummary& summary) {
        deployedFiles_ = 0;
        bytesSaved_ = 0;
        emit telemetryChanged();
        emit notification("info", tr("Restored: %1 links safely unlinked.").arg(summary.removedFiles));
    });

    connect(&client_, &SmmClient::operationSucceeded, this, [this](const QString& title, const QString& detail) {
        emit notification("success", QString("%1: %2").arg(title, detail));
        // Mutating client operations refresh their own affected models.
    });

    connect(&client_, &SmmClient::operationFailed, this, [this](const QString& title, const QString& message) {
        emit notification("error", QString("%1: %2").arg(title, message));
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
    }
}

void GuiController::setStagingDir(const QString& dir) {
    if (stagingDir_ != dir) {
        stagingDir_ = dir;
        client_.setStagingDir(dir);
        checkVolumeMatch();
        emit stagingDirChanged();
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
    client_.refreshDoctor();
    updateDeploymentTelemetry();
}

void GuiController::deploy() {
    client_.deploy();
}

void GuiController::deployAndLaunch() {
    client_.deploy();
    launchGame();
}

void GuiController::restore() {
    client_.restore();
}

void GuiController::launchGame() {
    // 1. 优先尝试直接启动本地配置的游戏可执行文件 sekiro.exe，脱离 Steam 依赖
    QString gameDir = sekiroDir_;
    if (gameDir.isEmpty() || !QDir(gameDir).exists()) {
        const QString detected = detectSekiroDir();
        if (!detected.isEmpty()) {
            gameDir = detected;
        }
    }

    if (!gameDir.isEmpty()) {
        const QString exePath = QDir(gameDir).filePath(QStringLiteral("sekiro.exe"));
        if (QFile::exists(exePath)) {
            bool started = QProcess::startDetached(exePath, QStringList{}, gameDir);
            if (started) {
                emit notification("success", tr("已直接启动只狼游戏 (sekiro.exe)"));
                return;
            }
        }
    }

    // 2. 本地 sekiro.exe 未找到时，再尝试通过 Steam 协议拉起 Sekiro (AppID 814380)
    bool opened = QDesktopServices::openUrl(QUrl("steam://rungameid/814380"));
    if (opened) {
        emit notification("info", tr("未在本地路径找到独立程序，已通过 Steam 协议拉起 (AppID 814380)"));
    } else {
        emit notification("warning", tr("启动失败：未检测到 sekiro.exe，且无法通过 Steam 拉起。请先在全局配置中指定只狼游戏目录。"));
    }
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
        list.append(m);
    }
    return list;
}

void GuiController::refreshDoctor() {
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
