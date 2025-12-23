#ifndef COUNTRYFILEDOWNLOADER_H
#define COUNTRYFILEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    void downloadFinished(bool success, const QString& filePath);
    void versionChecked(int latestVersion);
    void errorOccurred(const QString& error);

private slots:
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onVersionCheckFinished();

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply{nullptr};
    QString m_saveDir;

    // Parse version number from HTML (e.g., "CTY-3540")
    int parseVersionFromHtml(const QString& html);
};

} // namespace TR4QT

#endif // COUNTRYFILEDOWNLOADER_H
