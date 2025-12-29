#include "CountryFileDownloader.h"
#include "../core/Constants.h"
#include "../logging/LogMacros.h"
#include "../3rdparty/miniz/miniz.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QTemporaryDir>

namespace TR4QT {

CountryFileDownloader::CountryFileDownloader(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

CountryFileDownloader::~CountryFileDownloader() {
    cancel();
}

void CountryFileDownloader::downloadLatest(const QString& saveDir) {
    m_saveDir = saveDir;

    // Ensure save directory exists
    QDir dir;
    if (!dir.mkpath(saveDir)) {
        emit errorOccurred("Failed to create directory: " + saveDir);
        return;
    }

    // First, check the latest version by fetching the homepage
    checkLatestVersion();
}

void CountryFileDownloader::checkLatestVersion() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    // Fetch RSS feed to get latest version
    QUrl rssUrl("https://www.country-files.com/feed/");
    QNetworkRequest request{rssUrl};

    // Set User-Agent header (some servers reject requests without it)
    request.setRawHeader("User-Agent", "TR4QT/2.33 (Amateur Radio Contest Logger)");

    LOG_DEBUG("CountryFileDownloader", "=== HTTP REQUEST ===");
    LOG_DEBUG("CountryFileDownloader", QString("URL: %1").arg(rssUrl.toString()));
    LOG_DEBUG("CountryFileDownloader", "Request headers:");
    for (const QByteArray& header : request.rawHeaderList()) {
        LOG_DEBUG("CountryFileDownloader", QString("  %1: %2").arg(QString::fromUtf8(header)).arg(QString::fromUtf8(request.rawHeader(header))));
    }

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &CountryFileDownloader::onVersionCheckFinished);
}

void CountryFileDownloader::onVersionCheckFinished() {
    if (!m_currentReply) return;

    // Log HTTP response details
    LOG_DEBUG("CountryFileDownloader", "=== HTTP RESPONSE ===");
    LOG_DEBUG("CountryFileDownloader", QString("HTTP Status: %1").arg(m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    LOG_DEBUG("CountryFileDownloader", QString("HTTP Reason: %1").arg(m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    LOG_DEBUG("CountryFileDownloader", "Response headers:");
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        LOG_DEBUG("CountryFileDownloader", QString("  %1: %2").arg(QString::fromUtf8(header.first)).arg(QString::fromUtf8(header.second)));
    }
    LOG_DEBUG("CountryFileDownloader", QString("Error code: %1").arg(m_currentReply->error()));
    LOG_DEBUG("CountryFileDownloader", QString("Error string: %1").arg(m_currentReply->errorString()));

    if (m_currentReply->error() != QNetworkReply::NoError) {
        // Log first 500 bytes of response body (might contain error details)
        QByteArray responseData = m_currentReply->readAll();
        LOG_DEBUG("CountryFileDownloader", QString("Response body (first 500 bytes): %1").arg(QString::fromUtf8(responseData.left(500))));

        emit errorOccurred("Failed to check version: " + m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QString rssXml = QString::fromUtf8(m_currentReply->readAll());
    LOG_DEBUG("CountryFileDownloader", QString("Response body size: %1 bytes").arg(rssXml.size()));
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    m_latestVersion = parseVersionFromRss(rssXml);

    if (!m_latestVersion.isEmpty()) {
        emit versionChecked(m_latestVersion);

        // Extract numerical version from VER string for download URL
        // e.g., "VER20251218" -> we need to find "CTY-3540" from the description
        // Actually, we need to parse the numerical version from the title for the URL
        QRegularExpression numRe(R"(CTY-(\d{4}))");
        QRegularExpressionMatch numMatch = numRe.match(rssXml);
        int numericalVersion = CURRENT_CTY_VERSION;  // fallback
        if (numMatch.hasMatch()) {
            numericalVersion = numMatch.captured(1).toInt();
        }

        // Now download the actual file (ZIP format)
        QString downloadUrl = QString("https://www.country-files.com/cty/download/%1/cty-%1.zip")
                                      .arg(numericalVersion);

        LOG_DEBUG("CountryFileDownloader", QString("Latest CTY version from RSS feed: %1").arg(m_latestVersion));
        LOG_DEBUG("CountryFileDownloader", QString("Downloading ZIP file: %1").arg(downloadUrl));

        QNetworkRequest request{QUrl(downloadUrl)};
        request.setRawHeader("User-Agent", "TR4QT/2.33 (Amateur Radio Contest Logger)");

        // Enable automatic redirect following
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                            QNetworkRequest::NoLessSafeRedirectPolicy);

        LOG_DEBUG("CountryFileDownloader", "=== FILE DOWNLOAD REQUEST ===");
        LOG_DEBUG("CountryFileDownloader", QString("URL: %1").arg(downloadUrl));
        LOG_DEBUG("CountryFileDownloader", "Request headers:");
        for (const QByteArray& header : request.rawHeaderList()) {
            LOG_DEBUG("CountryFileDownloader", QString("  %1: %2").arg(QString::fromUtf8(header)).arg(QString::fromUtf8(request.rawHeader(header))));
        }

        m_currentReply = m_networkManager->get(request);

        connect(m_currentReply, &QNetworkReply::finished,
                this, &CountryFileDownloader::onDownloadFinished);
        connect(m_currentReply, &QNetworkReply::downloadProgress,
                this, &CountryFileDownloader::onDownloadProgress);
    } else {
        emit errorOccurred("Failed to parse latest version from RSS feed");
    }
}

void CountryFileDownloader::onDownloadFinished() {
    if (!m_currentReply) return;

    // Log HTTP response details
    LOG_DEBUG("CountryFileDownloader", "=== FILE DOWNLOAD RESPONSE ===");
    LOG_DEBUG("CountryFileDownloader", QString("Final URL: %1").arg(m_currentReply->url().toString()));
    LOG_DEBUG("CountryFileDownloader", QString("HTTP Status: %1").arg(m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
    LOG_DEBUG("CountryFileDownloader", QString("HTTP Reason: %1").arg(m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    LOG_DEBUG("CountryFileDownloader", "Response headers:");
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        LOG_DEBUG("CountryFileDownloader", QString("  %1: %2").arg(QString::fromUtf8(header.first)).arg(QString::fromUtf8(header.second)));
    }
    LOG_DEBUG("CountryFileDownloader", QString("Error code: %1").arg(m_currentReply->error()));
    LOG_DEBUG("CountryFileDownloader", QString("Error string: %1").arg(m_currentReply->errorString()));

    // Check if there were any redirects
    QVariant redirectTarget = m_currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectTarget.isNull()) {
        LOG_DEBUG("CountryFileDownloader", QString("Redirect target: %1").arg(redirectTarget.toUrl().toString()));
    }

    bool success = false;
    QString filePath;

    if (m_currentReply->error() == QNetworkReply::NoError) {
        QByteArray zipData = m_currentReply->readAll();
        LOG_DEBUG("CountryFileDownloader", QString("Downloaded %1 bytes (ZIP file)").arg(zipData.size()));

        // Create temporary directory for extraction
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            emit errorOccurred("Failed to create temporary directory");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        // Save ZIP to temporary file
        QString zipPath = tempDir.path() + "/cty.zip";
        QFile zipFile(zipPath);
        if (!zipFile.open(QIODevice::WriteOnly)) {
            emit errorOccurred("Failed to write ZIP file: " + zipPath);
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }
        zipFile.write(zipData);
        zipFile.close();
        LOG_DEBUG("CountryFileDownloader", QString("Saved ZIP to %1").arg(zipPath));

        // Extract ZIP file using miniz
        LOG_DEBUG("CountryFileDownloader", "Extracting ZIP with miniz");

        mz_zip_archive zip_archive;
        memset(&zip_archive, 0, sizeof(zip_archive));

        // Open ZIP file
        if (!mz_zip_reader_init_file(&zip_archive, zipPath.toUtf8().constData(), 0)) {
            emit errorOccurred("Failed to open ZIP archive");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        // Find cty.dat (case-insensitive search)
        int file_index = -1;
        int num_files = mz_zip_reader_get_num_files(&zip_archive);

        for (int i = 0; i < num_files; i++) {
            mz_zip_archive_file_stat file_stat;
            if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) {
                QString filename = QString::fromUtf8(file_stat.m_filename).toLower();
                if (filename.endsWith("cty.dat")) {
                    file_index = i;
                    LOG_DEBUG("CountryFileDownloader", QString("Found cty.dat at index %1: %2").arg(i).arg(file_stat.m_filename));
                    break;
                }
            }
        }

        if (file_index == -1) {
            mz_zip_reader_end(&zip_archive);
            emit errorOccurred("cty.dat not found in ZIP archive");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        // Extract to temp directory
        QString extractedPath = tempDir.path() + "/cty.dat";
        if (!mz_zip_reader_extract_to_file(&zip_archive, file_index, extractedPath.toUtf8().constData(), 0)) {
            mz_zip_reader_end(&zip_archive);
            emit errorOccurred("Failed to extract cty.dat from ZIP");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        mz_zip_reader_end(&zip_archive);
        LOG_DEBUG("CountryFileDownloader", QString("Successfully extracted to %1").arg(extractedPath));

        // Move extracted cty.dat to final location
        filePath = m_saveDir + "/" + COUNTRY_FILE_NAME;

        // Remove old file if it exists
        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }

        if (QFile::copy(extractedPath, filePath)) {
            success = true;
            LOG_DEBUG("CountryFileDownloader", QString("Downloaded and extracted country file to %1").arg(filePath));
        } else {
            emit errorOccurred("Failed to copy extracted file to: " + filePath);
        }
    } else {
        // Log first 500 bytes of error response
        QByteArray responseData = m_currentReply->readAll();
        LOG_DEBUG("CountryFileDownloader", QString("Error response body (first 500 bytes): %1").arg(QString::fromUtf8(responseData.left(500))));
        emit errorOccurred("Download failed: " + m_currentReply->errorString());
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    emit downloadFinished(success, filePath, m_latestVersion);
}

void CountryFileDownloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(bytesReceived, bytesTotal);
}

void CountryFileDownloader::cancel() {
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

QString CountryFileDownloader::parseVersionFromRss(const QString& rssXml) {
    // Parse RSS feed using QXmlStreamReader
    // Looking for the first <item><description> which contains "VER20251218, Version entity is Libya..."
    QXmlStreamReader xml(rssXml);

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == QStringLiteral("item")) {
            // Found an item, now look for description
            while (!xml.atEnd()) {
                xml.readNext();

                if (xml.isStartElement() && xml.name() == QStringLiteral("description")) {
                    QString description = xml.readElementText();
                    LOG_DEBUG("CountryFileDownloader", QString("RSS feed first item description: %1").arg(description));

                    // Extract version string from description like "VER20251218, Version entity is Libya, 5A Download: [ CTY-3540"
                    static QRegularExpression re(R"(VER\d{8})");
                    QRegularExpressionMatch match = re.match(description);

                    if (match.hasMatch()) {
                        QString version = match.captured(0);
                        LOG_DEBUG("CountryFileDownloader", QString("Parsed version from RSS description: %1").arg(version));
                        return version;
                    }
                }

                // Break when we reach end of first item
                if (xml.isEndElement() && xml.name() == QStringLiteral("item")) {
                    break;
                }
            }
            // Only process first item
            break;
        }
    }

    if (xml.hasError()) {
        LOG_WARN("CountryFileDownloader", QString("XML parsing error: %1").arg(xml.errorString()));
    }

    // Fallback: return empty string if parsing fails
    LOG_DEBUG("CountryFileDownloader", "Failed to parse version from RSS description");
    return QString();
}

} // namespace TR4QT
