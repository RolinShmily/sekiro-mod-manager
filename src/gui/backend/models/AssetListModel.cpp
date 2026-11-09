#include "backend/models/AssetListModel.hpp"

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

AssetListModel::AssetListModel(QObject* parent) : QAbstractListModel(parent) {}

int AssetListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return assets_.size();
}

QVariant AssetListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= assets_.size()) return {};
    const auto& asset = assets_.at(index.row());
    switch (role) {
        case RelativePathRole: return asset.relativePath;
        case CategoryRole: return asset.category;
        case FileSizeRole: return asset.fileSize;
        case CriticalRole: return asset.critical;
        case ExclusiveSlotRole: return asset.exclusiveSlot;
        case EnabledRole: return asset.enabled;
        case FormattedSizeRole: return formatBytes(asset.fileSize);
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
    for (const auto& asset : assets_) {
        if (asset.enabled) ++count;
    }
    return count;
}

void AssetListModel::toggleAsset(int row) {
    if (row < 0 || row >= assets_.size()) return;
    assets_[row].enabled = !assets_[row].enabled;
    const QModelIndex cell = index(row, 0);
    emit dataChanged(cell, cell, {EnabledRole});
    emit countsChanged(activeCount(), totalCount());
    emit assetToggleRequested(currentModId_, assets_[row].relativePath, assets_[row].enabled);
}

} // namespace smm::gui
