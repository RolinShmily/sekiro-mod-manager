#include "UpdateManager.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QLocale>

namespace {

const QString kDefaultManifestUrl = QStringLiteral(
    "https://github.com/RolinShmily/sekiro-mod-manager/releases/latest/download/latest.json"
);

} // namespace

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)) {}

UpdateManager::~UpdateManager() {
    if (replyManifest_) {
        replyManifest_->abort();
        replyManifest_->deleteLater();
        replyManifest_ = nullptr;
    }
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
    if (isChecking_) return;

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

    // Construct download and release URLs
    const QString tag = latestVersion_.startsWith('v', Qt::CaseInsensitive)
                        ? latestVersion_
                        : (QStringLiteral("v") + latestVersion_);
    releasePageUrl_ = QStringLiteral("https://github.com/RolinShmily/sekiro-mod-manager/releases/tag/") + tag;
    setupDownloadUrl_ = QStringLiteral("https://github.com/RolinShmily/sekiro-mod-manager/releases/download/")
                        + tag + QStringLiteral("/sekiro-mod-manager-") + tag + QStringLiteral("-windows-x64-gui-setup.exe");

    // Localized release notes
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

void UpdateManager::openReleasePage() {
    QString url = releasePageUrl_;
    if (url.isEmpty()) {
        url = QStringLiteral("https://github.com/RolinShmily/sekiro-mod-manager/releases");
    }
    QDesktopServices::openUrl(QUrl(url));
}

void UpdateManager::openSetupDownload() {
    QString url = setupDownloadUrl_;
    if (url.isEmpty()) {
        openReleasePage();
        return;
    }
    QDesktopServices::openUrl(QUrl(url));
}
