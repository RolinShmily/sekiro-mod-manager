#include "UpdateManager.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProcess>
#include <QCoreApplication>
#include <QLocale>

namespace {

const QString kDefaultManifestUrl = QStringLiteral(
    "https://github.com/RolinShmily/sekiro-mod-manager/releases/latest/download/latest.json"
);

} // namespace

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)) {}

UpdateManager::~UpdateManager() {
    cancelDownload();
}

bool UpdateManager::isNewerVersion(const QString& remote, const QString& current) {
    QString r = remote.trimmed();
    if (r.startsWith('v', Qt::CaseInsensitive)) r.remove(0, 1);
    QString c = current.trimmed();
    if (c.startsWith('v', Qt::CaseInsensitive)) c.remove(0, 1);

    const auto rParts = r.split('.');
    const auto cParts = c.split('.');
    const int count = std::max(rParts.size(), cParts.size());

    for (int i = 0; i < count; ++i) {
        int rVal = i < rParts.size() ? rParts[i].toInt() : 0;
        int cVal = i < cParts.size() ? cParts[i].toInt() : 0;
        if (rVal > cVal) return true;
        if (rVal < cVal) return false;
    }
    return false;
}

void UpdateManager::checkForUpdates(bool silent) {
    if (isChecking_ || isDownloading_) return;

    silentCheck_ = silent;
    isChecking_ = true;
    emit checkingChanged();

    statusMessage_ = tr("Checking for updates...");
    emit statusMessageChanged();

    QNetworkRequest request;
    request.setUrl(QUrl(kDefaultManifestUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "SekiroModManager/" SMM_VERSION_STRING);

    if (replyManifest_) {
        replyManifest_->deleteLater();
        replyManifest_ = nullptr;
    }

    replyManifest_ = network_->get(request);
    connect(replyManifest_, &QNetworkReply::finished, this, &UpdateManager::onManifestFinished);
}

void UpdateManager::onManifestFinished() {
    if (!replyManifest_) return;

    isChecking_ = false;
    emit checkingChanged();

    if (replyManifest_->error() != QNetworkReply::NoError) {
        const QString err = replyManifest_->errorString();
        replyManifest_->deleteLater();
        replyManifest_ = nullptr;

        statusMessage_ = tr("Update check failed: %1").arg(err);
        emit statusMessageChanged();

        if (!silentCheck_) {
            emit updateNotification("error", statusMessage_);
        }
        return;
    }

    const QByteArray data = replyManifest_->readAll();
    replyManifest_->deleteLater();
    replyManifest_ = nullptr;

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        statusMessage_ = tr("Failed to parse update manifest.");
        emit statusMessageChanged();
        if (!silentCheck_) {
            emit updateNotification("error", statusMessage_);
        }
        return;
    }

    const QJsonObject obj = doc.object();
    latestVersion_ = obj.value("version").toString();
    releaseUrl_ = obj.value("url").toString();
    releaseSha256_ = obj.value("sha256").toString().trimmed().toLower();

    // Check localized release notes
    const QString locale = QLocale::system().name();
    if (locale.startsWith("zh", Qt::CaseInsensitive) && obj.contains("notes_zh")) {
        releaseNotes_ = obj.value("notes_zh").toString();
    } else if (obj.contains("notes_en")) {
        releaseNotes_ = obj.value("notes_en").toString();
    } else {
        releaseNotes_ = obj.value("notes").toString();
    }

    if (isNewerVersion(latestVersion_, currentVersion())) {
        updateAvailable_ = true;
        statusMessage_ = tr("New version %1 is available!").arg(latestVersion_);
        emit updateAvailableChanged();
        emit statusMessageChanged();
        emit updateNotification("info", statusMessage_);
    } else {
        updateAvailable_ = false;
        statusMessage_ = tr("Sekiro Mod Manager is up to date (v%1).").arg(currentVersion());
        emit updateAvailableChanged();
        emit statusMessageChanged();
        if (!silentCheck_) {
            emit updateNotification("success", statusMessage_);
        }
    }
}

void UpdateManager::startDownload() {
    if (releaseUrl_.isEmpty() || isDownloading_) return;

    downloadedZipPath_ = QDir::temp().filePath(QStringLiteral("smm_update_%1.zip").arg(latestVersion_));
    if (targetFile_) {
        targetFile_->close();
        delete targetFile_;
        targetFile_ = nullptr;
    }

    targetFile_ = new QFile(downloadedZipPath_, this);
    if (!targetFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        statusMessage_ = tr("Cannot create local update file.");
        emit statusMessageChanged();
        emit updateNotification("error", statusMessage_);
        delete targetFile_;
        targetFile_ = nullptr;
        return;
    }

    isDownloading_ = true;
    downloadCompleted_ = false;
    downloadProgress_ = 0.0;
    emit downloadingChanged();
    emit downloadCompletedChanged();
    emit downloadProgressChanged();

    statusMessage_ = tr("Downloading update package...");
    emit statusMessageChanged();

    QNetworkRequest request;
    request.setUrl(QUrl(releaseUrl_));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "SekiroModManager/" SMM_VERSION_STRING);

    if (replyDownload_) {
        replyDownload_->deleteLater();
        replyDownload_ = nullptr;
    }

    replyDownload_ = network_->get(request);
    connect(replyDownload_, &QNetworkReply::readyRead, this, &UpdateManager::onDownloadData);
    connect(replyDownload_, &QNetworkReply::downloadProgress, this, &UpdateManager::onDownloadProgress);
    connect(replyDownload_, &QNetworkReply::finished, this, &UpdateManager::onDownloadFinished);
}

void UpdateManager::onDownloadData() {
    if (replyDownload_ && targetFile_) {
        targetFile_->write(replyDownload_->readAll());
    }
}

void UpdateManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        downloadProgress_ = static_cast<qreal>(bytesReceived) / static_cast<qreal>(bytesTotal);
        emit downloadProgressChanged();
    }
}

void UpdateManager::onDownloadFinished() {
    if (!replyDownload_) return;

    isDownloading_ = false;
    emit downloadingChanged();

    if (targetFile_) {
        targetFile_->flush();
        targetFile_->close();
    }

    if (replyDownload_->error() != QNetworkReply::NoError) {
        const QString err = replyDownload_->errorString();
        replyDownload_->deleteLater();
        replyDownload_ = nullptr;

        statusMessage_ = tr("Download failed: %1").arg(err);
        emit statusMessageChanged();
        emit updateNotification("error", statusMessage_);
        return;
    }

    replyDownload_->deleteLater();
    replyDownload_ = nullptr;

    // Check SHA-256 integrity if provided
    if (!releaseSha256_.isEmpty()) {
        QFile verifyFile(downloadedZipPath_);
        if (verifyFile.open(QIODevice::ReadOnly)) {
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (hash.addData(&verifyFile)) {
                const QString actualSha256 = QString::fromLatin1(hash.result().toHex()).toLower();
                if (actualSha256 != releaseSha256_) {
                    statusMessage_ = tr("Integrity verification failed (SHA-256 mismatch).");
                    emit statusMessageChanged();
                    emit updateNotification("error", statusMessage_);
                    verifyFile.close();
                    verifyFile.remove();
                    return;
                }
            }
        }
    }

    downloadCompleted_ = true;
    downloadProgress_ = 1.0;
    statusMessage_ = tr("Update ready to install. Click to restart.");
    emit downloadCompletedChanged();
    emit downloadProgressChanged();
    emit statusMessageChanged();
    emit updateNotification("success", statusMessage_);
}

void UpdateManager::cancelDownload() {
    if (replyDownload_) {
        replyDownload_->abort();
        replyDownload_->deleteLater();
        replyDownload_ = nullptr;
    }
    if (targetFile_) {
        targetFile_->close();
        targetFile_->remove();
        delete targetFile_;
        targetFile_ = nullptr;
    }
    if (isDownloading_) {
        isDownloading_ = false;
        emit downloadingChanged();
    }
}

void UpdateManager::applyUpdateAndRestart() {
    if (!downloadCompleted_ || downloadedZipPath_.isEmpty()) return;

    const QString appDir = QCoreApplication::applicationDirPath();

    // Locate smm.exe
    QString cliPath = appDir + "/smm.exe";
    if (!QFileInfo::exists(cliPath)) {
        cliPath = appDir + "/../cli/smm.exe";
    }
    if (!QFileInfo::exists(cliPath)) {
        cliPath = appDir + "/../cli/Debug/smm.exe";
    }
    if (!QFileInfo::exists(cliPath)) {
        cliPath = appDir + "/../cli/Release/smm.exe";
    }

    if (!QFileInfo::exists(cliPath)) {
        statusMessage_ = tr("Cannot find smm.exe updater tool.");
        emit statusMessageChanged();
        emit updateNotification("error", statusMessage_);
        return;
    }

    const QStringList args = {
        QStringLiteral("self-update"),
        QStringLiteral("--zip"), downloadedZipPath_,
        QStringLiteral("--wait-pid"), QString::number(QCoreApplication::applicationPid()),
        QStringLiteral("--target-dir"), appDir,
        QStringLiteral("--restart")
    };

    const bool launched = QProcess::startDetached(cliPath, args);
    if (!launched) {
        statusMessage_ = tr("Failed to launch updater process.");
        emit statusMessageChanged();
        emit updateNotification("error", statusMessage_);
        return;
    }

    // Exit application immediately so Windows releases file locks
    QCoreApplication::quit();
}
