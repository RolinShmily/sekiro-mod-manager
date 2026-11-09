#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "backend/GuiTypes.hpp"

namespace smm::gui {

/// QML-facing list model for saved modpack presets.
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

} // namespace smm::gui
