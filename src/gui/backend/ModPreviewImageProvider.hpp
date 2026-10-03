#pragma once

#include <QQuickImageProvider>
#include <QCache>
#include <QImage>
#include <QMutex>
#include <QString>

namespace smm::gui {

class GuiController;

/// High-performance QQuickImageProvider decoding WebP, PNG, JPG, and BMP
/// with Windows hardware WIC, on-demand downsampling, and LRU memory cache.
class ModPreviewImageProvider : public QQuickImageProvider {
public:
    explicit ModPreviewImageProvider(GuiController* controller);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    static void clearCache();

private:
    GuiController* controller_{nullptr};
    static QCache<QString, QImage> s_cache;
    static QMutex s_cacheMutex;
};

} // namespace smm::gui
