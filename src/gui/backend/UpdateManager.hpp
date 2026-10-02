#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QUrl>

class UpdateManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isChecking READ isChecking NOTIFY checkingChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool downloadCompleted READ isDownloadCompleted NOTIFY downloadCompletedChanged)

public:
    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager() override;

    bool isChecking() const { return isChecking_; }
    bool isDownloading() const { return isDownloading_; }
    bool isUpdateAvailable() const { return updateAvailable_; }
    QString currentVersion() const { return QStringLiteral("0.3.1"); }
    QString latestVersion() const { return latestVersion_; }
    QString releaseNotes() const { return releaseNotes_; }
    QString releaseUrl() const { return releaseUrl_; }
    qreal downloadProgress() const { return downloadProgress_; }
    QString statusMessage() const { return statusMessage_; }
    bool isDownloadCompleted() const { return downloadCompleted_; }

    Q_INVOKABLE void checkForUpdates(bool silent = false);
    Q_INVOKABLE void startDownload();
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void applyUpdateAndRestart();

signals:
    void checkingChanged();
    void downloadingChanged();
    void updateAvailableChanged();
    void downloadProgressChanged();
    void statusMessageChanged();
    void downloadCompletedChanged();
    void updateNotification(const QString& type, const QString& message);

private slots:
    void onManifestFinished();
    void onDownloadData();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    static bool isNewerVersion(const QString& remote, const QString& current);

    QNetworkAccessManager* network_{nullptr};
    QNetworkReply* replyManifest_{nullptr};
    QNetworkReply* replyDownload_{nullptr};
    QFile* targetFile_{nullptr};

    bool isChecking_{false};
    bool isDownloading_{false};
    bool updateAvailable_{false};
    bool downloadCompleted_{false};
    bool silentCheck_{false};

    QString latestVersion_;
    QString releaseNotes_;
    QString releaseUrl_;
    QString releaseSha256_;
    qreal downloadProgress_{0.0};
    QString statusMessage_;
    QString downloadedZipPath_;
};
