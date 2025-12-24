#ifndef DXCLUSTERLISTDOWNLOADER_H
#define DXCLUSTERLISTDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>

namespace TR4QT {

/**
 * DX Cluster server entry
 */
struct DXClusterServer {
    QString callsign;      // e.g., "W9ODD"
    QString host;          // e.g., "134.48.91.82" or "dxc.nc7j.com"
    int port;              // e.g., 23 or 7373
    QString clusterType;   // e.g., "AR-Cluster", "CC Cluster", "DXSpider"
    QString country;       // Derived from callsign prefix

    // Format for display in combo box: "W9ODD (134.48.91.82:23) - AR-Cluster"
    QString displayString() const {
        return QString("%1 (%2:%3) - %4")
            .arg(callsign)
            .arg(host)
            .arg(port)
            .arg(clusterType);
    }

    // Format for connection: "host:port"
    QString connectionString() const {
        return QString("%1:%2").arg(host).arg(port);
    }
};

/**
 * DX Cluster List Downloader
 *
 * Downloads and parses DXCLUSTERS.DAT from dxcluster.info
 * Format: "CALLSIGN","HOST","PORT","TYPE"
 * Example: "W9ODD","134.48.91.82","23","AR-Cluster"
 */
class DXClusterListDownloader : public QObject {
    Q_OBJECT

public:
    explicit DXClusterListDownloader(QObject* parent = nullptr);
    ~DXClusterListDownloader() override;

    /**
     * Download the DX cluster list from dxcluster.info
     */
    void downloadList();

    /**
     * Cancel ongoing download
     */
    void cancel();

    /**
     * Parse DXCLUSTERS.DAT format
     * Returns list of cluster servers
     */
    static QList<DXClusterServer> parseClusterList(const QString& data);

    /**
     * Sort cluster list by country, with user's country first
     * @param servers List to sort (modified in place)
     * @param userCallsign User's callsign to determine home country
     */
    static void sortByCountry(QList<DXClusterServer>& servers, const QString& userCallsign);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QList<DXClusterServer>& servers);
    void errorOccurred(const QString& error);

private slots:
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_currentReply{nullptr};

    // Extract country prefix from callsign (e.g., "W9ODD" -> "K" for USA)
    static QString getCountryPrefix(const QString& callsign);
};

} // namespace TR4QT

#endif // DXCLUSTERLISTDOWNLOADER_H
