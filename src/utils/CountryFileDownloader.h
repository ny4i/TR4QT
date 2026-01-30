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
    void downloadFinished(bool success, const QString& filePath, const QString& version, int numericalVersion);
    void versionChecked(const QString& latestVersion);
    void updateAvailable(int currentVersion, int latestVersion, const QString& versionString);
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
    int m_numericalVersion{0};  // Store numerical CTY version (e.g., 3541)
    bool m_shouldDownload{false};  // True if version check should proceed to download

    // Parse version string from RSS feed (e.g., "VER20251218" from description)
    QString parseVersionFromRss(const QString& rssXml);
};

} // namespace TR4QT

#endif // COUNTRYFILEDOWNLOADER_H
