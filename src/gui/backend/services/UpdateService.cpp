#include "backend/services/UpdateService.hpp"

#include "backend/services/UpdateFeed.hpp"

#include <QDesktopServices>
#include <QLocale>
#include <QNetworkRequest>

namespace smm::gui {

UpdateService::UpdateService(QObject* parent)
    : QObject(parent), network_(new QNetworkAccessManager(this)) {}

UpdateService::~UpdateService() {
    if (reply_ != nullptr) {
        reply_->abort();
        reply_->deleteLater();
        reply_ = nullptr;
    }
}

void UpdateService::setStatus(const QString& message) {
    statusMessage_ = message;
    emit statusMessageChanged();
}

void UpdateService::fail(const QString& message, bool notify) {
    setStatus(message);
    if (notify) {
        emit updateNotification(QStringLiteral("error"), message);
    }
}

void UpdateService::fetchManifest(const QString& url) {
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "SekiroModManager/" SMM_VERSION_STRING);
    request.setTransferTimeout(10000);

    if (reply_ != nullptr) {
        reply_->deleteLater();
        reply_ = nullptr;
    }

    reply_ = network_->get(request);
    connect(reply_, &QNetworkReply::finished, this, &UpdateService::onManifestFinished);
}

void UpdateService::checkForUpdates(bool silent) {
    if (isChecking_) {
        return;
    }

    silentCheck_ = silent;
    usingFallback_ = false;
    isChecking_ = true;
    emit checkingChanged();
    setStatus(tr("Checking for updates..."));

    fetchManifest(QString::fromLatin1(UpdateFeed::kMirrorManifestUrl));
}

void UpdateService::onManifestFinished() {
    if (reply_ == nullptr) {
        return;
    }

    const QNetworkReply::NetworkError netErr = reply_->error();
    const QString netErrStr = reply_->errorString();
    const QByteArray body = netErr == QNetworkReply::NoError ? reply_->readAll() : QByteArray{};
    reply_->deleteLater();
    reply_ = nullptr;

    std::string parseError;
    const auto manifest =
        netErr == QNetworkReply::NoError ? UpdateFeed::parse(body.toStdString(), &parseError) : std::nullopt;

    if (!manifest.has_value()) {
        if (!usingFallback_) {
            usingFallback_ = true;
            fetchManifest(QString::fromLatin1(UpdateFeed::kGithubManifestUrl));
            return;
        }

        isChecking_ = false;
        emit checkingChanged();
        if (netErr != QNetworkReply::NoError) {
            fail(tr("Update check failed: %1").arg(netErrStr), !silentCheck_);
        } else {
            fail(tr("Failed to parse update manifest."), !silentCheck_);
        }
        return;
    }

    isChecking_ = false;
    emit checkingChanged();

    const auto links = UpdateFeed::linksFor(*manifest, !usingFallback_);
    latestVersion_ = QString::fromStdString(manifest->version);
    releasePageUrl_ = QString::fromStdString(links.releasePage);
    setupDownloadUrl_ = QString::fromStdString(links.setupDownload);
    releaseNotes_ = QString::fromStdString(
        UpdateFeed::notesFor(*manifest, QLocale::system().name().toStdString()));

    const bool newer =
        UpdateFeed::isNewer(manifest->version, currentVersion().toStdString());
    updateAvailable_ = newer;
    emit updateAvailableChanged();

    if (newer) {
        setStatus(tr("New version %1 is available!").arg(latestVersion_));
        emit updateNotification(QStringLiteral("info"), statusMessage_);
    } else {
        setStatus(tr("Sekiro Mod Manager is up to date (v%1).").arg(currentVersion()));
        if (!silentCheck_) {
            emit updateNotification(QStringLiteral("success"), statusMessage_);
        }
    }
}

void UpdateService::openReleasePage() {
    const QString url = releasePageUrl_.isEmpty()
                            ? QStringLiteral("https://github.com/RolinShmily/sekiro-mod-manager/releases")
                            : releasePageUrl_;
    QDesktopServices::openUrl(QUrl(url));
}

void UpdateService::openSetupDownload() {
    if (setupDownloadUrl_.isEmpty()) {
        openReleasePage();
        return;
    }
    QDesktopServices::openUrl(QUrl(setupDownloadUrl_));
}

} // namespace smm::gui
