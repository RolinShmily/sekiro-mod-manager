#include "backend/models/ModListModel.hpp"

#include <algorithm>

namespace smm::gui {

namespace {

QString formatBytes(quint64 bytes) {
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0), 'f', 2) +
               " GB";
    }
    if (bytes >= 1024ULL * 1024ULL) {
        return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1) + " MB";
    }
    if (bytes >= 1024ULL) {
        return QString::number(static_cast<double>(bytes) / 1024.0, 'f', 0) + " KB";
    }
    return QString::number(bytes) + " B";
}

} // namespace

ModListModel::ModListModel(QObject* parent) : QAbstractListModel(parent) {}

int ModListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return visibleIndices_.size();
}

QVector<int> ModListModel::filteredIndices(const QVector<ModEntry>& mods) const {
    QVector<int> indices;
    indices.reserve(mods.size());
    const QString filter = filterText_.trimmed();
    const QString category = selectedCategory_.trimmed();
    for (int i = 0; i < mods.size(); ++i) {
        const auto& mod = mods[i];
        if (onlyEquippedPackMods_ && !equippedModIds_.contains(mod.id)) {
            continue;
        }
        if (!category.isEmpty() && category != QLatin1String("all") &&
            mod.category.compare(category, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (!filter.isEmpty() && !mod.name.contains(filter, Qt::CaseInsensitive) &&
            !mod.author.contains(filter, Qt::CaseInsensitive) &&
            !mod.category.contains(filter, Qt::CaseInsensitive) &&
            !mod.id.contains(filter, Qt::CaseInsensitive) &&
            !mod.description.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }
        indices.append(i);
    }
    return indices;
}

void ModListModel::rebuildFilter() {
    visibleIndices_ = filteredIndices(mods_);
}

void ModListModel::setSelectedCategory(const QString& category) {
    if (selectedCategory_ == category) return;
    beginResetModel();
    selectedCategory_ = category;
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
    const auto& mod = mods_.at(realIndex);
    switch (role) {
        case IdRole: return mod.id;
        case NameRole: return mod.name;
        case VersionRole: return mod.version;
        case AuthorRole: return mod.author;
        case CategoryRole: return mod.category;
        case DescriptionRole: return mod.description;
        case HomepageRole: return mod.homepage;
        case SourceUrlRole: return mod.sourceUrl;
        case EnabledRole: return mod.enabled;
        case PriorityRole: return mod.priority;
        case AssetCountRole: return mod.assetCount;
        case TotalBytesRole: return formatBytes(mod.totalBytes);
        case RootPathRole: return mod.rootPath;
        case PreviewImageRole: return mod.previewImage;
        case PreviewImagePathRole:
            return !mod.previewImage.isEmpty() && !mod.rootPath.isEmpty()
                       ? QStringLiteral("image://modpreview/") + mod.id
                       : QString{};
        case DisabledAssetsRole: return mod.disabledAssets;
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
    const bool sameOrder =
        sorted.size() == mods_.size() &&
        std::equal(sorted.cbegin(), sorted.cend(), mods_.cbegin(),
                   [](const ModEntry& a, const ModEntry& b) { return a.id == b.id; });
    // Metadata edits can change search/category membership even when IDs stay ordered.
    QVector<int> updatedIndices = filteredIndices(sorted);
    const bool resetRequired = !sameOrder || updatedIndices != visibleIndices_;

    if (resetRequired) beginResetModel();
    mods_ = std::move(sorted);
    visibleIndices_ = std::move(updatedIndices);

    QStringList categories;
    categories.append(QStringLiteral("all"));
    for (const auto& mod : mods_) {
        if (!mod.category.isEmpty() && !categories.contains(mod.category)) {
            categories.append(mod.category);
        }
    }
    const bool categoriesChanged = availableCategories_ != categories;
    availableCategories_ = categories;

    if (resetRequired) {
        endResetModel();
        emit countChanged();
    } else if (rowCount() > 0) {
        emit dataChanged(index(0), index(rowCount() - 1));
    }
    if (categoriesChanged) emit availableCategoriesChanged();
}

const ModEntry* ModListModel::findMod(const QString& id) const {
    for (const auto& mod : mods_) {
        if (mod.id == id) return &mod;
    }
    return nullptr;
}

bool ModListModel::rowToRealIndex(int row, int* realIndex) const {
    if (realIndex == nullptr || row < 0 || row >= visibleIndices_.size()) {
        return false;
    }
    const int real = visibleIndices_.at(row);
    if (real < 0 || real >= mods_.size()) {
        return false;
    }
    *realIndex = real;
    return true;
}

QStringList ModListModel::allModIds() const {
    QStringList ids;
    ids.reserve(mods_.size());
    for (const auto& mod : mods_) {
        ids.append(mod.id);
    }
    return ids;
}

void ModListModel::toggleEnabled(int row) {
    int realIndex = -1;
    if (!rowToRealIndex(row, &realIndex)) return;
    auto& mod = mods_[realIndex];
    mod.enabled = !mod.enabled;
    const QModelIndex cell = index(row, 0);
    emit dataChanged(cell, cell, {EnabledRole});
    emit enabledToggleRequested(mod.id, mod.enabled);
}

void ModListModel::swapPriority(int rowA, int rowB) {
    int realA = -1;
    int realB = -1;
    if (rowA == rowB || !rowToRealIndex(rowA, &realA) || !rowToRealIndex(rowB, &realB)) return;

    const QString idA = mods_[realA].id;
    const QString idB = mods_[realB].id;
    // Swapping the rows means each id takes over the other row's priority.
    const quint32 newPriorityOfA = mods_[realB].priority;
    const quint32 newPriorityOfB = mods_[realA].priority;

    beginResetModel();
    std::swap(mods_[realA], mods_[realB]);
    rebuildFilter();
    endResetModel();

    emit prioritySwapRequested(idA, newPriorityOfA, idB, newPriorityOfB);
}

void ModListModel::moveRow(int from, int to) {
    int realFrom = -1;
    int realTo = -1;
    if (from == to || !rowToRealIndex(from, &realFrom) || !rowToRealIndex(to, &realTo)) return;

    // Move the row locally so the drag feels instant; priorities are re-normalised by the
    // controller against core, and the refresh that follows is authoritative.
    if (!beginMoveRows({}, from, from, {}, to > from ? to + 1 : to)) return;
    mods_.move(realFrom, realTo);
    rebuildFilter();
    endMoveRows();

    QStringList orderedIds;
    orderedIds.reserve(mods_.size());
    for (const auto& mod : mods_) {
        orderedIds.append(mod.id);
    }
    emit reorderRequested(orderedIds);
}

} // namespace smm::gui
