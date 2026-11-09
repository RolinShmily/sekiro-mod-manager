#include "backend/models/PresetListModel.hpp"

namespace smm::gui {

PresetListModel::PresetListModel(QObject* parent) : QAbstractListModel(parent) {}

int PresetListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return presets_.size();
}

QVariant PresetListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= presets_.size()) return {};
    const auto& preset = presets_.at(index.row());
    switch (role) {
        case IdRole: return preset.id;
        case NameRole: return preset.name;
        case DescriptionRole: return preset.description;
        case NameEnRole: return preset.nameEn.isEmpty() ? preset.name : preset.nameEn;
        case DescriptionEnRole:
            return preset.descriptionEn.isEmpty() ? preset.description : preset.descriptionEn;
        case ModCountRole: return preset.modCount;
        case UpdatedAtRole: return preset.updatedAt;
        case IsEquippedRole: return preset.id == equippedId_;
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
    if (equippedId_ == id) return;
    // The reset has to bracket the change, otherwise views keep roles computed from the
    // previous selection (the equipped badge used to lag one click behind).
    beginResetModel();
    equippedId_ = id;
    endResetModel();
}

} // namespace smm::gui
