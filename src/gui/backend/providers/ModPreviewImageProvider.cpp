#include "backend/providers/ModPreviewImageProvider.hpp"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <utility>
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

/// Decodes `filePath` at (approximately) the requested cover size.
///
/// WIC can scale while decoding, so a 4K preview that a 480x270 card will only ever show is
/// never materialised as a 4K BGRA buffer. Decoding at display size cuts both the peak memory
/// (a 3840x2160 frame is 33 MB) and the CPU spent on the smooth resample the caller used to do
/// afterwards. `targetW`/`targetH` follow KeepAspectRatioByExpanding: the result covers the
/// requested box, and images smaller than the request are left untouched (never upscaled).
QImage decode_image_wic(const std::wstring& filePath, int targetW, int targetH) {
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

    // Scale during decode when a target box is known and the source is bigger than it.
    IWICBitmapScaler* pScaler = NULL;
    IWICBitmapSource* pSource = pFrame;
    if (targetW > 0 && targetH > 0) {
        UINT srcW = 0, srcH = 0;
        if (SUCCEEDED(pFrame->GetSize(&srcW, &srcH)) && srcW > 0 && srcH > 0 &&
            (srcW > static_cast<UINT>(targetW) || srcH > static_cast<UINT>(targetH))) {
            const double scale = std::max(static_cast<double>(targetW) / srcW,
                                          static_cast<double>(targetH) / srcH);
            UINT dstW = static_cast<UINT>(std::lround(srcW * scale));
            UINT dstH = static_cast<UINT>(std::lround(srcH * scale));
            dstW = std::min(std::max(dstW, static_cast<UINT>(targetW)), srcW);
            dstH = std::min(std::max(dstH, static_cast<UINT>(targetH)), srcH);
            if (SUCCEEDED(pFactory->CreateBitmapScaler(&pScaler)) &&
                SUCCEEDED(pScaler->Initialize(pFrame, dstW, dstH, WICBitmapInterpolationModeFant))) {
                pSource = pScaler;
            }
        }
    }

    IWICFormatConverter* pConverter = NULL;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) {
        if (pScaler) pScaler->Release();
        pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return {};
    }

    hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        pConverter->Release();
        if (pScaler) pScaler->Release();
        pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return {};
    }

    UINT width = 0, height = 0;
    hr = pConverter->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) {
        pConverter->Release();
        if (pScaler) pScaler->Release();
        pFrame->Release(); pDecoder->Release(); pFactory->Release();
        return {};
    }

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    const UINT stride = width * 4;
    const UINT bufferSize = stride * height;
    hr = pConverter->CopyPixels(NULL, stride, bufferSize, image.bits());

    pConverter->Release();
    if (pScaler) pScaler->Release();
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

ModPreviewImageProvider::ModPreviewImageProvider(StagingDirResolver stagingDir)
    : QQuickImageProvider(QQuickImageProvider::Image), stagingDir_(std::move(stagingDir)) {
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
    const QString staging = stagingDir_ ? stagingDir_() : QString{};
    if (checkDirect.isAbsolute() && checkDirect.exists()) {
        targetFilePath = checkDirect.absoluteFilePath();
    } else if (!staging.isEmpty()) {
        const QDir modDir(QDir(staging).filePath(cleanId));
        if (modDir.exists()) {
            // preview.webp is what ModManager writes, so it is tried first.
            if (modDir.exists(QStringLiteral("preview.webp"))) {
                targetFilePath = modDir.filePath(QStringLiteral("preview.webp"));
            } else {
                const QStringList candidates = {
                    QStringLiteral("preview.png"), QStringLiteral("preview.jpg"),
                    QStringLiteral("preview.jpeg"), QStringLiteral("cover.webp"),
                    QStringLiteral("cover.png"), QStringLiteral("cover.jpg")};
                for (const auto& candidate : candidates) {
                    if (modDir.exists(candidate)) {
                        targetFilePath = modDir.filePath(candidate);
                        break;
                    }
                }
                if (targetFilePath.isEmpty()) {
                    // Last resort: any image the pack shipped at its root.
                    const auto images = modDir.entryList(
                        {QStringLiteral("*.webp"), QStringLiteral("*.png"), QStringLiteral("*.jpg"),
                         QStringLiteral("*.jpeg"), QStringLiteral("*.bmp")},
                        QDir::Files);
                    if (!images.isEmpty()) {
                        targetFilePath = modDir.filePath(images.first());
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
    loaded = decode_image_wic(QDir::toNativeSeparators(targetFilePath).toStdWString(), reqW, reqH);
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
