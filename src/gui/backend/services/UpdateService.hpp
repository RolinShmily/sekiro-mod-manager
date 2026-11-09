#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

namespace smm::gui {

/// GUI-side update *service*: performs the network request, owns the transient check state
/// and exposes ready-made strings for the QML dialog.
///
/// Pure logic (manifest parsing, version comparison, link construction) lives in UpdateFeed and
/// is unit tested there; this class is the async shell around it. It never touches QML objects:
/// the dialog binds to properties and reacts to `updateNotification`, which GuiController
/// forwards (or drops) like any other notification.
class UpdateService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isChecking READ isChecking NOTIFY checkingChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString releasePageUrl READ releasePageUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString setupDownloadUrl READ setupDownloadUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit UpdateService(QObject* parent = nullptr);
    ~UpdateService() override;

    bool isChecking() const { return isChecking_; }
    bool isUpdateAvailable() const { return updateAvailable_; }
    QString currentVersion() const { return QStringLiteral(SMM_VERSION_STRING); }
    QString latestVersion() const { return latestVersion_; }
    QString releaseNotes() const { return releaseNotes_; }
    QString releasePageUrl() const { return releasePageUrl_; }
    QString setupDownloadUrl() const { return setupDownloadUrl_; }
    QString statusMessage() const { return statusMessage_; }

    Q_INVOKABLE void checkForUpdates(bool silent = false);
    Q_INVOKABLE void openReleasePage();
    Q_INVOKABLE void openSetupDownload();

signals:
    void checkingChanged();
    void updateAvailableChanged();
    void statusMessageChanged();
    void updateNotification(const QString& type, const QString& message);

private:
    void onManifestFinished();
    void fail(const QString& message, bool notify);
    void setStatus(const QString& message);

    QNetworkAccessManager* network_{nullptr};
    QNetworkReply* reply_{nullptr};

    bool isChecking_{false};
    bool updateAvailable_{false};
    bool silentCheck_{false};

    QString latestVersion_;
    QString releaseNotes_;
    QString releasePageUrl_;
    QString setupDownloadUrl_;
    QString statusMessage_;
};

} // namespace smm::gui
