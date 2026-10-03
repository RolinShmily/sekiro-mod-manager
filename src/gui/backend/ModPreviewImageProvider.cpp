#include "ModPreviewImageProvider.hpp"
#include "Models.hpp"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <smm/manager.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace {

QImage decode_image_wic(const std::wstring& filePath) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr)) return {};

    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(filePath.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr)) { pFactory->Release(); return {}; }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { pDecoder->Release(); pFactory->Release(); return {}; }

    IWICFormatConverter* pConverter = NULL;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { pFrame->Release(); pDecoder->Release(); pFactory->Release(); return {}; }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return {}; }

    UINT width = 0, height = 0;
    hr = pConverter->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) {
        pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return {};
    }

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    const UINT stride = width * 4;
    const UINT bufferSize = stride * height;
    hr = pConverter->CopyPixels(NULL, stride, bufferSize, image.bits());

    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();

    if (FAILED(hr)) return {};
    return image;
}

} // namespace
#endif

namespace smm::gui {

QCache<QString, QImage> ModPreviewImageProvider::s_cache(128 * 1024); // 128 MB cache limit
QMutex ModPreviewImageProvider::s_cacheMutex;

ModPreviewImageProvider::ModPreviewImageProvider(GuiController* controller)
    : QQuickImageProvider(QQuickImageProvider::Image), controller_(controller) {
}

void ModPreviewImageProvider::clearCache() {
    QMutexLocker locker(&s_cacheMutex);
    s_cache.clear();
}

QImage ModPreviewImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    // 剥离 query 参数（如 ?rev=1）
    QString cleanId = id;
    const int queryIndex = cleanId.indexOf('?');
    if (queryIndex != -1) {
        cleanId = cleanId.left(queryIndex);
    }

    cleanId = QUrl::fromPercentEncoding(cleanId.toUtf8()).trimmed();
    if (cleanId.isEmpty()) {
        return {};
    }

    QString targetFilePath;
    QFileInfo checkDirect(cleanId);
    if (checkDirect.isAbsolute() && checkDirect.exists()) {
        targetFilePath = checkDirect.absoluteFilePath();
    } else if (controller_) {
        const QString staging = controller_->stagingDir();
        if (!staging.isEmpty()) {
            const QDir modDir(QDir(staging).filePath(cleanId));
            if (modDir.exists()) {
                // 优先寻找规范的 preview.webp
                if (modDir.exists(QStringLiteral("preview.webp"))) {
                    targetFilePath = modDir.filePath(QStringLiteral("preview.webp"));
                } else {
                    // 寻找其他候选名称
                    const QStringList candidates = {
                        QStringLiteral("preview.png"),
                        QStringLiteral("preview.jpg"),
                        QStringLiteral("preview.jpeg"),
                        QStringLiteral("cover.webp"),
                        QStringLiteral("cover.png"),
                        QStringLiteral("cover.jpg")
                    };
                    for (const auto& cand : candidates) {
                        if (modDir.exists(cand)) {
                            targetFilePath = modDir.filePath(cand);
                            break;
                        }
                    }
                    if (targetFilePath.isEmpty()) {
                        // 寻找任意图片
                        const auto list = modDir.entryList(
                            {QStringLiteral("*.webp"), QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.bmp")},
                            QDir::Files);
                        if (!list.isEmpty()) {
                            targetFilePath = modDir.filePath(list.first());
                        }
                    }
                }
            }
        }
    }

    if (targetFilePath.isEmpty() || !QFile::exists(targetFilePath)) {
        return {};
    }

    const int reqW = requestedSize.width() > 0 ? requestedSize.width() : 0;
    const int reqH = requestedSize.height() > 0 ? requestedSize.height() : 0;
    const QString cacheKey = targetFilePath + QStringLiteral("@") + QString::number(reqW) + QStringLiteral("x") + QString::number(reqH);

    {
        QMutexLocker locker(&s_cacheMutex);
        if (QImage* cached = s_cache.object(cacheKey)) {
            if (size) *size = cached->size();
            return *cached;
        }
    }

    QImage loaded;
#ifdef _WIN32
    loaded = decode_image_wic(QDir::toNativeSeparators(targetFilePath).toStdWString());
#endif
    if (loaded.isNull()) {
        loaded.load(targetFilePath);
    }

    if (loaded.isNull()) {
        return {};
    }

    if (size) {
        *size = loaded.size();
    }

    // 内存与渲染降采样优化：根据 QML 请求尺寸（如卡片 480x270）在后台平滑缩放
    QImage result = loaded;
    if (reqW > 0 && reqH > 0 && (loaded.width() > reqW || loaded.height() > reqH)) {
        result = loaded.scaled(QSize(reqW, reqH), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    // 存入 LRU 缓存（按 KB 计费）
    const int costKb = static_cast<int>(result.sizeInBytes() / 1024);
    {
        QMutexLocker locker(&s_cacheMutex);
        s_cache.insert(cacheKey, new QImage(result), std::max(1, costKb));
    }

    return result;
}

} // namespace smm::gui
