#ifndef SCPDOWNLOADER_H
#define SCPDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace TR4QT {

/**
 * Downloads MASTER.SCP from supercheckpartial.com
 *
 * Downloads and imports the Super Check Partial database file containing
 * active contester callsigns. The download process:
 *
 * 1. Check http://supercheckpartial.com/history.htm for latest version
 * 2. Download http://www.supercheckpartial.com/MASTER.SCP if newer version available
 * 3. Parse plain text file (one callsign per line)
 * 4. Bulk import to GlobalDatabase via SCPRepository
 *
 * File format: Plain text, one callsign per line, no header
 */
class SCPDownloader : public QObject {
    Q_OBJECT

public:
    explicit SCPDownloader(QObject* parent = nullptr);
    ~SCPDownloader() override;

    /**
     * Download the latest MASTER.SCP file
     * Downloads from http://www.supercheckpartial.com/MASTER.SCP
     * Automatically parses and imports to database
     */
    void downloadLatest();

    /**
     * Check what the latest version is (from history.htm)
     * Emits versionChecked signal with version string
     */
    void checkLatestVersion();

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
     * @param callsignCount Number of callsigns imported (0 if failed)
     * @param errorMsg Error message if success=false, empty otherwise
     */
    void downloadFinished(bool success, int callsignCount, const QString& errorMsg);

    /**
     * Version check completed
     * @param latestVersion Version string from history.htm (e.g., "20250115")
     */
    void versionChecked(const QString& latestVersion);

    /**
     * Error occurred during download or import
     * @param error Error description
     */
    void errorOccurred(const QString& error);

private slots:
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onVersionCheckFinished();

private:
    /**
     * Parse MASTER.SCP text data and import to database
     * Format: One callsign per line (plain text), no header
     *
     * @param scpData Raw file content
     * @return Number of callsigns imported, -1 on error
     */
    int importToDatabase(const QByteArray& scpData);

    /**
     * Parse version from history.htm HTML
     * Looks for first <a href="MASTER.SCP"> link and extracts nearby date
     *
     * @param html HTML content from history.htm
     * @return Version string (e.g., "20250115"), empty if not found
     */
    QString parseVersionFromHistory(const QByteArray& html);

    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply{nullptr};
    QString m_latestVersion;  // Store latest version for comparison
};

} // namespace TR4QT

#endif // SCPDOWNLOADER_H
