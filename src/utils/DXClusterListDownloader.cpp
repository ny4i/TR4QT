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

#include "DXClusterListDownloader.h"
#include "../utils/CountryFile.h"
#include "../logging/LogMacros.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QRegularExpression>
#include <algorithm>

namespace TR4QT {

// URL for DX cluster list
static const char* DXCLUSTERS_URL = "http://www.dxcluster.info/telnet/DXCLUSTERS.DAT";

DXClusterListDownloader::DXClusterListDownloader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

DXClusterListDownloader::~DXClusterListDownloader() {
    cancel();
}

void DXClusterListDownloader::downloadList() {
    if (m_currentReply) {
        LOG_WARN("DXClusterListDownloader", "Download already in progress");
        return;
    }

    QUrl url(DXCLUSTERS_URL);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "TR4QT/2.0 (Amateur Radio Contest Logger)");
    request.setRawHeader("Accept", "*/*");

    LOG_DEBUG("DXClusterListDownloader", QString("Downloading DX cluster list from: %1").arg(DXCLUSTERS_URL));

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &DXClusterListDownloader::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &DXClusterListDownloader::onDownloadProgress);
}

void DXClusterListDownloader::cancel() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void DXClusterListDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void DXClusterListDownloader::onDownloadFinished() {
    if (!m_currentReply) {
        return;
    }

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        QString error = QString("Download failed: %1").arg(reply->errorString());
        LOG_WARN("DXClusterListDownloader", error);
        emit errorOccurred(error);
        emit downloadFinished(false, QList<DXClusterServer>());
        reply->deleteLater();
        return;
    }

    // Read downloaded data
    QByteArray data = reply->readAll();
    reply->deleteLater();

    LOG_DEBUG("DXClusterListDownloader", QString("Downloaded %1 bytes").arg(data.size()));

    // Parse cluster list
    QString dataStr = QString::fromUtf8(data);
    QList<DXClusterServer> servers = parseClusterList(dataStr);

    LOG_DEBUG("DXClusterListDownloader", QString("Parsed %1 cluster servers").arg(servers.size()));

    emit downloadFinished(true, servers);
}

QList<DXClusterServer> DXClusterListDownloader::parseClusterList(const QString& data) {
    QList<DXClusterServer> servers;

    // Parse CSV format: "CALLSIGN","HOST","PORT","TYPE"
    // Example: "W9ODD","134.48.91.82","23","AR-Cluster"

    QStringList lines = data.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;  // Skip empty lines and comments
        }

        // Parse CSV with quoted fields
        QRegularExpression csvRegex("\"([^\"]*)\",\"([^\"]*)\",\"([^\"]*)\",\"([^\"]*)\"");
        QRegularExpressionMatch match = csvRegex.match(trimmed);

        if (match.hasMatch()) {
            DXClusterServer server;
            server.callsign = match.captured(1);
            server.host = match.captured(2);
            bool ok;
            server.port = match.captured(3).toInt(&ok);
            if (!ok || server.port <= 0 || server.port > 65535) {
                LOG_WARN("DXClusterListDownloader", QString("Invalid port in line: %1").arg(trimmed));
                continue;
            }
            server.clusterType = match.captured(4);
            server.country = getCountryPrefix(server.callsign);

            servers.append(server);
        } else {
            LOG_WARN("DXClusterListDownloader", QString("Failed to parse line: %1").arg(trimmed));
        }
    }

    return servers;
}

void DXClusterListDownloader::sortByCountry(QList<DXClusterServer>& servers, const QString& userCallsign) {
    if (servers.isEmpty()) {
        return;
    }

    // Get user's country prefix
    QString userCountry = getCountryPrefix(userCallsign);

    // Sort: user's country first, then alphabetically by country, then by callsign
    std::sort(servers.begin(), servers.end(), [&userCountry](const DXClusterServer& a, const DXClusterServer& b) {
        // User's country always comes first
        bool aIsUserCountry = (a.country == userCountry);
        bool bIsUserCountry = (b.country == userCountry);

        if (aIsUserCountry && !bIsUserCountry) return true;
        if (!aIsUserCountry && bIsUserCountry) return false;

        // If both same country status, sort by country name
        if (a.country != b.country) {
            return a.country < b.country;
        }

        // Same country, sort by callsign
        return a.callsign < b.callsign;
    });
}

QString DXClusterListDownloader::getCountryPrefix(const QString& callsign) {
    // Simple prefix extraction - could be enhanced with CountryFile lookup
    // For now, extract first letter(s) before digits

    QRegularExpression prefixRegex("^([A-Z0-9]+?)\\d");
    QRegularExpressionMatch match = prefixRegex.match(callsign);

    if (match.hasMatch()) {
        QString prefix = match.captured(1);

        // Handle special cases
        if (prefix.startsWith('K') || prefix.startsWith('W') ||
            prefix.startsWith('N') || prefix.startsWith('A')) {
            return "K";  // USA
        }
        if (prefix.startsWith("VE") || prefix.startsWith("VA") ||
            prefix.startsWith("VO") || prefix.startsWith("VY")) {
            return "VE";  // Canada
        }
        if (prefix.startsWith('G') || prefix.startsWith('M')) {
            return "G";  // UK
        }
        if (prefix.startsWith("DL") || prefix.startsWith("DA") ||
            prefix.startsWith("DB") || prefix.startsWith("DC")) {
            return "DL";  // Germany
        }
        if (prefix.startsWith('F')) {
            return "F";  // France
        }
        if (prefix.startsWith("JA") || prefix.startsWith("JE") ||
            prefix.startsWith("JF") || prefix.startsWith("JH")) {
            return "JA";  // Japan
        }

        return prefix;
    }

    return "UNKNOWN";
}

} // namespace TR4QT
