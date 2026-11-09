#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "backend/GuiTypes.hpp"

namespace smm::gui {

/// QML-facing list model for the individual asset files of one mod.
///
/// Same contract as ModListModel: rows and toggle state only. Enabling an asset is a file
/// write in core (`ModManager::set_asset_enabled`), so it is emitted as a request.
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
    void countsChanged(int active, int total);
    void assetToggleRequested(const QString& modId, const QString& relativePath, bool enabled);

private:
    QString currentModId_;
    QVector<AssetEntry> assets_;
};

} // namespace smm::gui
