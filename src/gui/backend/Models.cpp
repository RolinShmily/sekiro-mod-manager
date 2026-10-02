#include "Models.hpp"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSettings>

#include <smm/loader.hpp>
#include <smm/manager.hpp>

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

void ModListModel::rebuildFilter() {
    visibleIndices_.clear();
    const QString f = filterText_.trimmed();
    const QString cat = selectedCategory_.trimmed();
    for (int i = 0; i < mods_.size(); ++i) {
        const auto& m = mods_[i];
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
        visibleIndices_.append(i);
    }
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
                const QString full = QDir(m.rootPath).filePath(m.previewImage);
                if (QFileInfo::exists(full)) {
                    return QUrl::fromLocalFile(full).toString();
                }
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
    beginResetModel();
    mods_ = mods;
    std::sort(mods_.begin(), mods_.end(), [](const ModEntry& a, const ModEntry& b) {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.id < b.id;
    });
    rebuildFilter();

    QStringList cats;
    cats.append(QStringLiteral("all"));
    for (const auto& m : mods_) {
        if (!m.category.isEmpty() && !cats.contains(m.category)) {
            cats.append(m.category);
        }
    }
    availableCategories_ = cats;

    endResetModel();
    emit countChanged();
    emit availableCategoriesChanged();
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

    beginResetModel();
    mods_.move(realFrom, realTo);
    QMap<QString, quint32> batch;
    for (int i = 0; i < mods_.size(); ++i) {
        mods_[i].priority = static_cast<quint32>((i + 1) * 10);
        batch.insert(mods_[i].id, mods_[i].priority);
    }
    rebuildFilter();
    endResetModel();

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

    if (!stagingDir_.isEmpty()) client_.setStagingDir(stagingDir_);
    if (!sekiroDir_.isEmpty()) client_.setGameDir(sekiroDir_);

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
        emit telemetryChanged();
        emit notification("info", tr("Restored: %1 links safely unlinked.").arg(summary.removedFiles));
    });

    connect(&client_, &SmmClient::operationSucceeded, this, [this](const QString& title, const QString& detail) {
        emit notification("success", QString("%1: %2").arg(title, detail));
        client_.refreshMods();
        client_.refreshPlan();
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
    emit languageChanged(language_);
}

void GuiController::refreshAll() {
    client_.refreshEnvironment();
    client_.refreshMods();
    client_.refreshConflicts();
    client_.refreshPlan();
    client_.refreshPresets();
    client_.refreshDoctor();
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
    // 优先通过 Steam 协议拉起 Sekiro (AppID 814380)
    QDesktopServices::openUrl(QUrl("steam://rungameid/814380"));
    emit notification("info", tr("Launched Sekiro via Steam (AppID 814380)"));
}

void GuiController::autoDetectGameDir() {
    // 典型 Steam 库默认安装路径
    const QStringList candidates = {
        "C:/Program Files (x86)/Steam/steamapps/common/Sekiro",
        "D:/SteamLibrary/steamapps/common/Sekiro",
        "E:/SteamLibrary/steamapps/common/Sekiro",
        "F:/SteamLibrary/steamapps/common/Sekiro"
    };
    for (const auto& path : candidates) {
        if (QFileInfo::exists(path + "/sekiro.exe")) {
            setSekiroDir(path);
            emit notification("success", tr("Auto-detected Sekiro installation: %1").arg(path));
            return;
        }
    }
    emit notification("warning", tr("Could not auto-detect Sekiro in default Steam libraries. Please select manually."));
}

void GuiController::saveSettings(const QString& staging, const QString& game) {
    setStagingDir(staging);
    setSekiroDir(game);
    client_.saveConfig(staging, game);

    QSettings settings("SekiroModManager", "SMM");
    settings.setValue("stagingDir", staging);
    settings.setValue("sekiroDir", game);

    emit notification("success", tr("Preferences saved successfully."));
}

void GuiController::openModDetail(const QString& modId) {
    client_.refreshModDetail(modId);
}

void GuiController::setModPreview(const QString& modId, const QString& imagePath) {
    client_.setModPreview(modId, imagePath);
}

void GuiController::saveModPack(const QString& nameZh, const QString& descZh,
                                const QString& nameEn, const QString& descEn) {
    Q_UNUSED(nameEn)
    Q_UNUSED(descEn)
    client_.savePreset(nameZh, descZh);
}

void GuiController::applyModPack(const QString& packId) {
    presetListModel_.setEquippedId(packId);
    client_.applyPreset(packId);
}

void GuiController::exportModPack(const QString& packId, const QString& outputPath) {
    Q_UNUSED(packId)
    QStringList ids;
    for (const auto& m : modListModel_.mods()) {
        if (m.enabled) ids.append(m.id);
    }
    client_.exportModPack(ids, outputPath, "SekiroModPack", "Custom Modpack exported from SMM");
}

void GuiController::importModPack(const QString& packPath) {
    client_.importModPack(packPath);
}

void GuiController::importArchive(const QString& archivePath) {
    client_.importPaths({archivePath}, {}, {}, true);
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

void GuiController::updateModMetadata(const QString& modId, const QString& name,
                                      const QString& author, const QString& version,
                                      const QString& category, const QString& description,
                                      const QString& sourceUrl) {
    client_.updateModMetadata(modId, name, author, version, category, description, sourceUrl);
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
