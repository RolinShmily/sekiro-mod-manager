#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QVector>

#include "backend/GuiTypes.hpp"

namespace smm::gui {

/// QML-facing list model for staged mods.
///
/// The model owns *presentation* state only: the rows it was given, the search/category
/// filter and the rank shown next to each row. It never touches the filesystem and never
/// persists anything: a user action is reported as a request signal, and GuiController
/// decides whether the request reaches the core service. That keeps the model safe to use
/// from QML bindings and makes a failed write recoverable by simply re-setting the model.
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
    void setSelectedCategory(const QString& category);

    QStringList availableCategories() const { return availableCategories_; }

    bool onlyEquippedPackMods() const { return onlyEquippedPackMods_; }
    void setOnlyEquippedPackMods(bool only);
    void setEquippedModIds(const QSet<QString>& ids);

    /// Replaces the rows. `mods` is not required to be ordered.
    void setMods(const QVector<ModEntry>& mods);
    const QVector<ModEntry>& mods() const { return mods_; }
    const ModEntry* findMod(const QString& id) const;

    Q_INVOKABLE void toggleEnabled(int row);
    Q_INVOKABLE void swapPriority(int rowA, int rowB);
    Q_INVOKABLE void moveRow(int from, int to);
    Q_INVOKABLE QStringList allModIds() const;

signals:
    void countChanged();
    void filterTextChanged();
    void selectedCategoryChanged();
    void availableCategoriesChanged();
    void onlyEquippedPackModsChanged();

    /// Requests for GuiController. Emitted after the optimistic local change, so the view
    /// updates immediately; the controller persists and refreshes.
    void enabledToggleRequested(const QString& modId, bool enabled);
    void prioritySwapRequested(const QString& modIdA, quint32 priorityA, const QString& modIdB,
                               quint32 priorityB);
    void reorderRequested(const QStringList& orderedIds);

private:
    void rebuildFilter();
    QVector<int> filteredIndices(const QVector<ModEntry>& mods) const;
    bool rowToRealIndex(int row, int* realIndex) const;

    QVector<ModEntry> mods_;
    QVector<int> visibleIndices_;
    QString filterText_;
    QString selectedCategory_{QStringLiteral("all")};
    QStringList availableCategories_;
    bool onlyEquippedPackMods_{false};
    QSet<QString> equippedModIds_;
};

} // namespace smm::gui
