/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

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
