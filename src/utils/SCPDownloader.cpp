#include "SCPDownloader.h"
#include "../data/SCPRepository.h"
#include "../logging/LogMacros.h"
#include <QRegularExpression>
#include <QDateTime>

namespace TR4QT {

SCPDownloader::SCPDownloader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

SCPDownloader::~SCPDownloader() {
    cancel();
}

void SCPDownloader::downloadLatest() {
    // Download MASTER.SCP directly
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    QUrl url("http://www.supercheckpartial.com/MASTER.SCP");
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "TR4QT/3.9 (Amateur Radio Contest Logger)");

    LOG_DEBUG("SCPDownloader", "=== HTTP REQUEST ===");
    LOG_DEBUG("SCPDownloader", QString("URL: %1").arg(url.toString()));
    LOG_DEBUG("SCPDownloader", "Request headers:");
    for (const QByteArray& header : request.rawHeaderList()) {
        LOG_DEBUG("SCPDownloader", QString("  %1: %2")
            .arg(QString::fromUtf8(header))
            .arg(QString::fromUtf8(request.rawHeader(header))));
    }

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &SCPDownloader::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &SCPDownloader::onDownloadProgress);
}

void SCPDownloader::checkLatestVersion() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    // Fetch history.htm to check latest version
    QUrl historyUrl("http://supercheckpartial.com/history.htm");
    QNetworkRequest request(historyUrl);
    request.setRawHeader("User-Agent", "TR4QT/3.9 (Amateur Radio Contest Logger)");

    LOG_DEBUG("SCPDownloader", "=== VERSION CHECK REQUEST ===");
    LOG_DEBUG("SCPDownloader", QString("URL: %1").arg(historyUrl.toString()));
    LOG_DEBUG("SCPDownloader", "Request headers:");
    for (const QByteArray& header : request.rawHeaderList()) {
        LOG_DEBUG("SCPDownloader", QString("  %1: %2")
            .arg(QString::fromUtf8(header))
            .arg(QString::fromUtf8(request.rawHeader(header))));
    }

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &SCPDownloader::onVersionCheckFinished);
}

void SCPDownloader::cancel() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void SCPDownloader::onDownloadFinished() {
    if (!m_currentReply) return;

    // Log HTTP response details
    LOG_DEBUG("SCPDownloader", "=== DOWNLOAD RESPONSE ===");
    LOG_DEBUG("SCPDownloader", QString("HTTP Status: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    LOG_DEBUG("SCPDownloader", QString("HTTP Reason: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    LOG_DEBUG("SCPDownloader", "Response headers:");
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        LOG_DEBUG("SCPDownloader", QString("  %1: %2")
            .arg(QString::fromUtf8(header.first))
            .arg(QString::fromUtf8(header.second)));
    }
    LOG_DEBUG("SCPDownloader", QString("Error code: %1").arg(m_currentReply->error()));
    LOG_DEBUG("SCPDownloader", QString("Error string: %1").arg(m_currentReply->errorString()));

    if (m_currentReply->error() != QNetworkReply::NoError) {
        // Log first 500 bytes of response body
        QByteArray responseData = m_currentReply->readAll();
        LOG_DEBUG("SCPDownloader", QString("Response body (first 500 bytes): %1")
            .arg(QString::fromUtf8(responseData.left(500))));

        QString errorMsg = QString("Download failed: %1").arg(m_currentReply->errorString());
        LOG_WARN("SCPDownloader", errorMsg);

        emit errorOccurred(errorMsg);
        emit downloadFinished(false, 0, errorMsg);

        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray scpData = m_currentReply->readAll();
    LOG_DEBUG("SCPDownloader", QString("Downloaded %1 bytes").arg(scpData.size()));

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    // Import to database
    int callsignCount = importToDatabase(scpData);

    if (callsignCount > 0) {
        LOG_INFO("SCPDownloader", QString("Successfully imported %1 callsigns").arg(callsignCount));
        emit downloadFinished(true, callsignCount, QString());
    } else {
        QString errorMsg = "Failed to import callsigns to database";
        LOG_WARN("SCPDownloader", errorMsg);
        emit downloadFinished(false, 0, errorMsg);
    }
}

void SCPDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void SCPDownloader::onVersionCheckFinished() {
    if (!m_currentReply) return;

    // Log HTTP response details
    LOG_DEBUG("SCPDownloader", "=== VERSION CHECK RESPONSE ===");
    LOG_DEBUG("SCPDownloader", QString("HTTP Status: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    LOG_DEBUG("SCPDownloader", QString("HTTP Reason: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));

    if (m_currentReply->error() != QNetworkReply::NoError) {
        QByteArray responseData = m_currentReply->readAll();
        LOG_DEBUG("SCPDownloader", QString("Response body (first 500 bytes): %1")
            .arg(QString::fromUtf8(responseData.left(500))));

        emit errorOccurred("Version check failed: " + m_currentReply->errorString());

        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QByteArray html = m_currentReply->readAll();
    LOG_DEBUG("SCPDownloader", QString("Downloaded history.htm: %1 bytes").arg(html.size()));

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    m_latestVersion = parseVersionFromHistory(html);

    if (!m_latestVersion.isEmpty()) {
        LOG_DEBUG("SCPDownloader", QString("Latest SCP version: %1").arg(m_latestVersion));
        emit versionChecked(m_latestVersion);
    } else {
        LOG_WARN("SCPDownloader", "Failed to parse version from history.htm");
        emit errorOccurred("Failed to parse latest version");
    }
}

int SCPDownloader::importToDatabase(const QByteArray& scpData) {
    // Parse callsigns (one per line)
    QString text = QString::fromUtf8(scpData);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);

    QStringList callsigns;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed().toUpper();

        // Skip empty lines and comments
        if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith("//")) {
            continue;
        }

        callsigns << trimmed;
    }

    LOG_DEBUG("SCPDownloader", QString("Parsed %1 callsigns from MASTER.SCP").arg(callsigns.size()));

    // Clear old master SCP data
    SCPRepository repo;
    int cleared = repo.clearBySource("master_scp");
    LOG_DEBUG("SCPDownloader", QString("Cleared %1 old callsigns").arg(cleared));

    // Bulk insert new data
    int inserted = repo.bulkInsert(callsigns, "master_scp");

    // Store metadata
    QString version = QDateTime::currentDateTime().toString("yyyyMMdd");
    repo.setMetadata("master_scp_version", version);
    repo.setMetadata("master_scp_count", QString::number(inserted));
    repo.setMetadata("master_scp_download_date", QDateTime::currentDateTime().toString(Qt::ISODate));

    LOG_INFO("SCPDownloader", QString("Imported %1 callsigns to SCP database").arg(inserted));

    return inserted;
}

QString SCPDownloader::parseVersionFromHistory(const QByteArray& html) {
    // Parse history.htm HTML to find latest version
    // Look for first occurrence of date pattern (e.g., "2025-01-15" or "20250115")
    // Or look for first <a href="MASTER.SCP"> link

    QString htmlText = QString::fromUtf8(html);

    // Strategy 1: Look for ISO date format (YYYY-MM-DD) in first few lines
    QRegularExpression isoDateRe(R"(\b(\d{4}-\d{2}-\d{2})\b)");
    QRegularExpressionMatch match = isoDateRe.match(htmlText);
    if (match.hasMatch()) {
        QString isoDate = match.captured(1);
        // Convert "2025-01-15" to "20250115"
        return isoDate.remove('-');
    }

    // Strategy 2: Look for compact date format (YYYYMMDD)
    QRegularExpression compactDateRe(R"(\b(\d{8})\b)");
    match = compactDateRe.match(htmlText);
    if (match.hasMatch()) {
        return match.captured(1);
    }

    // Strategy 3: Use current date as fallback
    LOG_WARN("SCPDownloader", "Could not parse version from history.htm, using current date");
    return QDateTime::currentDateTime().toString("yyyyMMdd");
}

} // namespace TR4QT
