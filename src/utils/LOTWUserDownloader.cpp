#include "LOTWUserDownloader.h"
#include "../data/GlobalDatabase.h"
#include "../data/LOTWUserRepository.h"
#include "../logging/LogMacros.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QDateTime>

namespace TR4QT {

// LOTW user activity CSV URL from ARRL
static const char* LOTW_USER_CSV_URL = "https://lotw.arrl.org/lotw-user-activity.csv";

LOTWUserDownloader::LOTWUserDownloader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

LOTWUserDownloader::~LOTWUserDownloader() {
    cancel();
}

void LOTWUserDownloader::downloadLatest() {
    if (m_currentReply) {
        LOG_WARN("LOTWUserDownloader", "Download already in progress");
        return;
    }

    // Check if global database is open
    GlobalDatabase& db = GlobalDatabase::instance();
    if (!db.isOpen()) {
        QString error = "Global database is not open. Cannot download LOTW users.";
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        emit downloadFinished(false, 0, error);
        return;
    }

    LOG_DEBUG("LOTWUserDownloader", "Starting LOTW user list download...");

    QUrl url(LOTW_USER_CSV_URL);
    QNetworkRequest request{url};

    // Set User-Agent header
    request.setRawHeader("User-Agent", "TR4QT/2.40 (Amateur Radio Contest Logger)");

    LOG_DEBUG("LOTWUserDownloader", "=== HTTP REQUEST ===");
    LOG_DEBUG("LOTWUserDownloader", QString("URL: %1").arg(url.toString()));
    LOG_DEBUG("LOTWUserDownloader", "Request headers:");
    for (const QByteArray& header : request.rawHeaderList()) {
        LOG_DEBUG("LOTWUserDownloader", QString("  %1: %2")
            .arg(QString::fromUtf8(header))
            .arg(QString::fromUtf8(request.rawHeader(header))));
    }

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &LOTWUserDownloader::onDownloadFinished);
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &LOTWUserDownloader::onDownloadProgress);
}

void LOTWUserDownloader::cancel() {
    if (m_currentReply) {
        LOG_DEBUG("LOTWUserDownloader", "Cancelling download...");
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void LOTWUserDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    LOG_DEBUG("LOTWUserDownloader", QString("Download progress: %1 / %2 bytes")
        .arg(bytesReceived).arg(bytesTotal));
    emit downloadProgress(bytesReceived, bytesTotal);
}

void LOTWUserDownloader::onDownloadFinished() {
    if (!m_currentReply) {
        return;
    }

    // Log HTTP response details
    LOG_DEBUG("LOTWUserDownloader", "=== HTTP RESPONSE ===");
    LOG_DEBUG("LOTWUserDownloader", QString("HTTP Status: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    LOG_DEBUG("LOTWUserDownloader", QString("HTTP Reason: %1")
        .arg(m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    LOG_DEBUG("LOTWUserDownloader", "Response headers:");
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        LOG_DEBUG("LOTWUserDownloader", QString("  %1: %2")
            .arg(QString::fromUtf8(header.first))
            .arg(QString::fromUtf8(header.second)));
    }
    LOG_DEBUG("LOTWUserDownloader", QString("Error code: %1").arg(m_currentReply->error()));
    LOG_DEBUG("LOTWUserDownloader", QString("Error string: %1").arg(m_currentReply->errorString()));

    // Check for network errors
    if (m_currentReply->error() != QNetworkReply::NoError) {
        // Log first 500 bytes of response body (might contain error details)
        QByteArray responseData = m_currentReply->readAll();
        LOG_DEBUG("LOTWUserDownloader", QString("Response body (first 500 bytes): %1")
            .arg(QString::fromUtf8(responseData.left(500))));

        QString error = QString("Download failed: %1").arg(m_currentReply->errorString());
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        emit downloadFinished(false, 0, error);

        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    // Read CSV data
    QByteArray csvData = m_currentReply->readAll();
    LOG_DEBUG("LOTWUserDownloader", QString("Downloaded %1 bytes of CSV data").arg(csvData.size()));

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    // Check if data is empty
    if (csvData.isEmpty()) {
        QString error = "Downloaded file is empty";
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        emit downloadFinished(false, 0, error);
        return;
    }

    // Import to database
    int userCount = importToDatabase(csvData);

    if (userCount < 0) {
        // Error already logged in importToDatabase
        QString error = "Failed to import LOTW users to database";
        emit downloadFinished(false, 0, error);
    } else {
        LOG_DEBUG("LOTWUserDownloader", QString("Successfully imported %1 LOTW users").arg(userCount));
        emit downloadFinished(true, userCount, QString());
    }
}

int LOTWUserDownloader::importToDatabase(const QByteArray& csvData) {
    LOG_DEBUG("LOTWUserDownloader", "Parsing CSV and importing to database...");

    GlobalDatabase& db = GlobalDatabase::instance();
    LOTWUserRepository repo;

    // Parse CSV
    QList<LOTWUser> users;
    QString csvText = QString::fromUtf8(csvData);
    QStringList lines = csvText.split('\n', Qt::SkipEmptyParts);

    int lineNumber = 0;
    int validLines = 0;
    int invalidLines = 0;
    QDateTime currentTime = QDateTime::currentDateTime();

    for (const QString& line : lines) {
        lineNumber++;

        // Skip empty lines
        if (line.trimmed().isEmpty()) {
            continue;
        }

        // Parse CSV line: callsign,date,time
        QStringList parts = line.split(',');
        if (parts.size() != 3) {
            invalidLines++;
            if (invalidLines <= 5) {  // Log first few errors
                LOG_WARN("LOTWUserDownloader", QString("Line %1: Invalid format (expected 3 fields, got %2): %3")
                    .arg(lineNumber).arg(parts.size()).arg(line));
            }
            continue;
        }

        QString callsign = parts[0].trimmed().toUpper();
        QString date = parts[1].trimmed();
        QString time = parts[2].trimmed();

        // Basic validation
        if (callsign.isEmpty()) {
            invalidLines++;
            if (invalidLines <= 5) {
                LOG_WARN("LOTWUserDownloader", QString("Line %1: Empty callsign").arg(lineNumber));
            }
            continue;
        }

        // Validate date format (YYYY-MM-DD = 10 chars)
        if (date.length() != 10) {
            invalidLines++;
            if (invalidLines <= 5) {
                LOG_WARN("LOTWUserDownloader", QString("Line %1: Invalid date format: %2").arg(lineNumber).arg(date));
            }
            continue;
        }

        // Validate time format (HH:MM:SS = 8 chars)
        if (time.length() != 8) {
            invalidLines++;
            if (invalidLines <= 5) {
                LOG_WARN("LOTWUserDownloader", QString("Line %1: Invalid time format: %2").arg(lineNumber).arg(time));
            }
            continue;
        }

        // Create user record
        LOTWUser user;
        user.callsign = callsign;
        user.lastUploadDate = date;
        user.lastUploadTime = time;
        user.lastUpdated = currentTime;

        users.append(user);
        validLines++;
    }

    LOG_DEBUG("LOTWUserDownloader", QString("Parsed %1 total lines: %2 valid, %3 invalid")
        .arg(lineNumber).arg(validLines).arg(invalidLines));

    if (users.isEmpty()) {
        QString error = "No valid LOTW users found in CSV file";
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        return -1;
    }

    // Log sample of first 5 and last 5 users
    int sampleCount = qMin(5, users.size());
    LOG_DEBUG("LOTWUserDownloader", QString("Sample - First %1 LOTW users:").arg(sampleCount));
    for (int i = 0; i < sampleCount; ++i) {
        const LOTWUser& user = users[i];
        LOG_DEBUG("LOTWUserDownloader", QString("  [%1] %2 - Last upload: %3 %4")
            .arg(i + 1)
            .arg(user.callsign)
            .arg(user.lastUploadDate)
            .arg(user.lastUploadTime));
    }

    if (users.size() > 10) {
        LOG_DEBUG("LOTWUserDownloader", QString("Sample - Last %1 LOTW users:").arg(sampleCount));
        for (int i = users.size() - sampleCount; i < users.size(); ++i) {
            const LOTWUser& user = users[i];
            LOG_DEBUG("LOTWUserDownloader", QString("  [%1] %2 - Last upload: %3 %4")
                .arg(i + 1)
                .arg(user.callsign)
                .arg(user.lastUploadDate)
                .arg(user.lastUploadTime));
        }
    }

    // Begin transaction for bulk import
    LOG_DEBUG("LOTWUserDownloader", "Starting database transaction for bulk import");
    if (!db.beginTransaction()) {
        QString error = QString("Failed to start database transaction: %1").arg(db.lastError());
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        return -1;
    }

    // Clear old data
    LOG_DEBUG("LOTWUserDownloader", "Clearing old LOTW user data from database");
    if (!repo.clearAll()) {
        db.rollbackTransaction();
        QString error = QString("Failed to clear old LOTW data: %1").arg(repo.lastError());
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        return -1;
    }
    LOG_DEBUG("LOTWUserDownloader", "Old LOTW data cleared successfully");

    // Bulk insert new data
    LOG_DEBUG("LOTWUserDownloader", QString("Inserting %1 LOTW users into database").arg(users.size()));
    if (!repo.bulkInsert(users)) {
        db.rollbackTransaction();
        QString error = QString("Failed to insert LOTW data: %1").arg(repo.lastError());
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        return -1;
    }
    LOG_DEBUG("LOTWUserDownloader", QString("Bulk insert complete: %1 users").arg(users.size()));

    // Commit transaction
    LOG_DEBUG("LOTWUserDownloader", "Committing database transaction");
    if (!db.commitTransaction()) {
        db.rollbackTransaction();
        QString error = QString("Failed to commit transaction: %1").arg(db.lastError());
        LOG_ERROR("LOTWUserDownloader", error);
        emit errorOccurred(error);
        return -1;
    }
    LOG_DEBUG("LOTWUserDownloader", "Transaction committed successfully");

    LOG_DEBUG("LOTWUserDownloader", QString("Successfully imported %1 LOTW users to database").arg(users.size()));
    return users.size();
}

} // namespace TR4QT
