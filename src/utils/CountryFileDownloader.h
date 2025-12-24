#ifndef COUNTRYFILEDOWNLOADER_H
#define COUNTRYFILEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryFile>

namespace TR4QT {

class CountryFileDownloader : public QObject {
    Q_OBJECT

public:
    explicit CountryFileDownloader(QObject* parent = nullptr);
    ~CountryFileDownloader() override;

    // Download the latest cty.dat file
    void downloadLatest(const QString& saveDir);

    // Check what the latest version number is (without downloading)
    void checkLatestVersion();

    // Cancel ongoing download
    void cancel();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& filePath, const QString& version);
    void versionChecked(const QString& latestVersion);
    void errorOccurred(const QString& error);

private slots:
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onVersionCheckFinished();

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply{nullptr};
    QString m_saveDir;
    QString m_latestVersion;  // Store latest version (e.g., "VER20251218")

    // Parse version string from RSS feed (e.g., "VER20251218" from description)
    QString parseVersionFromRss(const QString& rssXml);
};

} // namespace TR4QT

#endif // COUNTRYFILEDOWNLOADER_H
