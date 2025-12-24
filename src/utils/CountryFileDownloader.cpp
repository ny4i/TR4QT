#include "CountryFileDownloader.h"
#include "../core/Constants.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QProcess>
#include <QTemporaryDir>
#include <QDebug>

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

    qDebug() << "=== HTTP REQUEST ===";
    qDebug() << "URL:" << rssUrl.toString();
    qDebug() << "Request headers:";
    for (const QByteArray& header : request.rawHeaderList()) {
        qDebug() << "  " << header << ":" << request.rawHeader(header);
    }

    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &CountryFileDownloader::onVersionCheckFinished);
}

void CountryFileDownloader::onVersionCheckFinished() {
    if (!m_currentReply) return;

    // Log HTTP response details
    qDebug() << "=== HTTP RESPONSE ===";
    qDebug() << "HTTP Status:" << m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP Reason:" << m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    qDebug() << "Response headers:";
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        qDebug() << "  " << header.first << ":" << header.second;
    }
    qDebug() << "Error code:" << m_currentReply->error();
    qDebug() << "Error string:" << m_currentReply->errorString();

    if (m_currentReply->error() != QNetworkReply::NoError) {
        // Log first 500 bytes of response body (might contain error details)
        QByteArray responseData = m_currentReply->readAll();
        qDebug() << "Response body (first 500 bytes):" << responseData.left(500);

        emit errorOccurred("Failed to check version: " + m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QString rssXml = QString::fromUtf8(m_currentReply->readAll());
    qDebug() << "Response body size:" << rssXml.size() << "bytes";
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

        qDebug() << "Latest CTY version from RSS feed:" << m_latestVersion;
        qDebug() << "Downloading ZIP file:" << downloadUrl;

        QNetworkRequest request{QUrl(downloadUrl)};
        request.setRawHeader("User-Agent", "TR4QT/2.33 (Amateur Radio Contest Logger)");

        // Enable automatic redirect following
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                            QNetworkRequest::NoLessSafeRedirectPolicy);

        qDebug() << "=== FILE DOWNLOAD REQUEST ===";
        qDebug() << "URL:" << downloadUrl;
        qDebug() << "Request headers:";
        for (const QByteArray& header : request.rawHeaderList()) {
            qDebug() << "  " << header << ":" << request.rawHeader(header);
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
    qDebug() << "=== FILE DOWNLOAD RESPONSE ===";
    qDebug() << "Final URL:" << m_currentReply->url().toString();
    qDebug() << "HTTP Status:" << m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP Reason:" << m_currentReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    qDebug() << "Response headers:";
    for (const QNetworkReply::RawHeaderPair& header : m_currentReply->rawHeaderPairs()) {
        qDebug() << "  " << header.first << ":" << header.second;
    }
    qDebug() << "Error code:" << m_currentReply->error();
    qDebug() << "Error string:" << m_currentReply->errorString();

    // Check if there were any redirects
    QVariant redirectTarget = m_currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectTarget.isNull()) {
        qDebug() << "Redirect target:" << redirectTarget.toUrl().toString();
    }

    bool success = false;
    QString filePath;

    if (m_currentReply->error() == QNetworkReply::NoError) {
        QByteArray zipData = m_currentReply->readAll();
        qDebug() << "Downloaded" << zipData.size() << "bytes (ZIP file)";

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
        qDebug() << "Saved ZIP to" << zipPath;

        // Extract ZIP file using system unzip command
        QProcess unzip;
        unzip.setWorkingDirectory(tempDir.path());
        unzip.start("unzip", QStringList() << "-o" << zipPath);

        if (!unzip.waitForFinished(10000)) {  // 10 second timeout
            emit errorOccurred("Unzip process timeout");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        if (unzip.exitCode() != 0) {
            QString error = QString("Unzip failed: %1").arg(QString::fromUtf8(unzip.readAllStandardError()));
            qDebug() << error;
            emit errorOccurred(error);
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        qDebug() << "Unzip output:" << QString::fromUtf8(unzip.readAllStandardOutput());

        // Find cty.dat in extracted files
        QDir extractedDir(tempDir.path());
        QFileInfoList files = extractedDir.entryInfoList(QStringList() << "cty.dat" << "CTY.DAT", QDir::Files);

        if (files.isEmpty()) {
            emit errorOccurred("cty.dat not found in ZIP archive");
            m_currentReply->deleteLater();
            m_currentReply = nullptr;
            emit downloadFinished(false, "", QString());
            return;
        }

        QString extractedFile = files.first().absoluteFilePath();
        qDebug() << "Found extracted file:" << extractedFile;

        // Move extracted cty.dat to final location
        filePath = m_saveDir + "/" + COUNTRY_FILE_NAME;

        // Remove old file if it exists
        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }

        if (QFile::copy(extractedFile, filePath)) {
            success = true;
            qDebug() << "Downloaded and extracted country file to" << filePath;
        } else {
            emit errorOccurred("Failed to copy extracted file to: " + filePath);
        }
    } else {
        // Log first 500 bytes of error response
        QByteArray responseData = m_currentReply->readAll();
        qDebug() << "Error response body (first 500 bytes):" << responseData.left(500);
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
                    qDebug() << "RSS feed first item description:" << description;

                    // Extract version string from description like "VER20251218, Version entity is Libya, 5A Download: [ CTY-3540"
                    static QRegularExpression re(R"(VER\d{8})");
                    QRegularExpressionMatch match = re.match(description);

                    if (match.hasMatch()) {
                        QString version = match.captured(0);
                        qDebug() << "Parsed version from RSS description:" << version;
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
        qWarning() << "XML parsing error:" << xml.errorString();
    }

    // Fallback: return empty string if parsing fails
    qDebug() << "Failed to parse version from RSS description";
    return QString();
}

} // namespace TR4QT
