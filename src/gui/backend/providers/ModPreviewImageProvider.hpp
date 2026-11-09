#pragma once

#include <QCache>
#include <QImage>
#include <QQuickImageProvider>
#include <QMutex>
#include <QString>

#include <functional>

namespace smm::gui {

/// Serves `image://modpreview/<mod-id>` from the staging directory.
///
/// Scope: locate an image file, decode it (WIC on Windows, Qt image plugins elsewhere), cache
/// and downsample. It deliberately knows nothing about controllers, models or mod metadata
/// rules - it is handed a way to ask for the current staging directory instead of reaching into
/// the GUI controller, so it cannot create a provider -> controller -> model cycle and cannot
/// outlive the object it asks.
class ModPreviewImageProvider : public QQuickImageProvider {
public:
    using StagingDirResolver = std::function<QString()>;

    explicit ModPreviewImageProvider(StagingDirResolver stagingDir);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    /// Drops decoded images, e.g. after a preview was replaced or a mod was deleted.
    static void clearCache();

private:
    StagingDirResolver stagingDir_;
    static QCache<QString, QImage> s_cache;
    static QMutex s_cacheMutex;
};

} // namespace smm::gui
