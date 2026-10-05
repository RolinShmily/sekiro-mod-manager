#pragma once

#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

class UpdateManager : public QObject {
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
    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager() override;

    bool isChecking() const { return isChecking_; }
    bool isUpdateAvailable() const { return updateAvailable_; }
    QString currentVersion() const { return QStringLiteral("0.3.6"); }
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

private slots:
    void onManifestFinished();

private:
    static bool isNewerVersion(const QString& remote, const QString& current);

    QNetworkAccessManager* network_{nullptr};
    QNetworkReply* replyManifest_{nullptr};

    bool isChecking_{false};
    bool updateAvailable_{false};
    bool silentCheck_{false};

    QString latestVersion_;
    QString releaseNotes_;
    QString releasePageUrl_;
    QString setupDownloadUrl_;
    QString statusMessage_;
};
