#ifndef LOTWUSERDOWNLOADER_H
#define LOTWUSERDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace TR4QT {

/**
 * Downloads the LOTW user list from ARRL
 *
 * Downloads CSV file from https://lotw.arrl.org/lotw-user-activity.csv
 * Format: callsign,YYYY-MM-DD,HH:MM:SS
 * Example: 1A0C,2016-04-23,20:15:52
 *
 * After download, automatically parses CSV and imports to global database.
 *
 * Usage:
 *   LOTWUserDownloader* downloader = new LOTWUserDownloader(this);
 *   connect(downloader, &LOTWUserDownloader::downloadFinished, ...);
 *   downloader->downloadLatest();
 */
class LOTWUserDownloader : public QObject {
    Q_OBJECT

public:
    explicit LOTWUserDownloader(QObject* parent = nullptr);
    ~LOTWUserDownloader() override;

    /**
     * Download the latest LOTW user CSV file
     * Downloads from https://lotw.arrl.org/lotw-user-activity.csv
     * Automatically parses and imports to database
     */
    void downloadLatest();

    /**
     * Cancel ongoing download
     */
    void cancel();

signals:
    /**
     * Progress during download
     * @param bytesReceived Bytes downloaded so far
     * @param bytesTotal Total bytes to download (may be 0 if unknown)
     */
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /**
     * Download and database import finished
     * @param success true if download and import succeeded
     * @param userCount Number of LOTW users imported (0 if failed)
     * @param errorMsg Error message if success=false, empty otherwise
     */
    void downloadFinished(bool success, int userCount, const QString& errorMsg);

    /**
     * Error occurred during download or import
     * @param error Error description
     */
    void errorOccurred(const QString& error);

private slots:
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    /**
     * Parse CSV data and import to database
     * CSV format: callsign,YYYY-MM-DD,HH:MM:SS
     *
     * @param csvData Raw CSV content
     * @return Number of users imported, -1 on error
     */
    int importToDatabase(const QByteArray& csvData);

    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply{nullptr};
};

} // namespace TR4QT

#endif // LOTWUSERDOWNLOADER_H
