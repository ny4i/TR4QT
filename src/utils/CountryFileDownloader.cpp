#include "CountryFileDownloader.h"
#include "../core/Constants.h"
#include <QFile>
#include <QDir>
#include <QRegularExpression>
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

    QNetworkRequest request{QUrl(COUNTRY_FILE_URL)};
    m_currentReply = m_networkManager->get(request);

    connect(m_currentReply, &QNetworkReply::finished,
            this, &CountryFileDownloader::onVersionCheckFinished);
}

void CountryFileDownloader::onVersionCheckFinished() {
    if (!m_currentReply) return;

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Failed to check version: " + m_currentReply->errorString());
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        return;
    }

    QString html = QString::fromUtf8(m_currentReply->readAll());
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    int latestVersion = parseVersionFromHtml(html);

    if (latestVersion > 0) {
        emit versionChecked(latestVersion);

        // Now download the actual file
        QString downloadUrl = QString("https://www.country-files.com/cty-%1/cty.dat")
                                      .arg(latestVersion);

        qDebug() << "Downloading" << downloadUrl;

        QNetworkRequest request{QUrl(downloadUrl)};
        m_currentReply = m_networkManager->get(request);

        connect(m_currentReply, &QNetworkReply::finished,
                this, &CountryFileDownloader::onDownloadFinished);
        connect(m_currentReply, &QNetworkReply::downloadProgress,
                this, &CountryFileDownloader::onDownloadProgress);
    } else {
        emit errorOccurred("Failed to parse latest version from website");
    }
}

void CountryFileDownloader::onDownloadFinished() {
    if (!m_currentReply) return;

    bool success = false;
    QString filePath;

    if (m_currentReply->error() == QNetworkReply::NoError) {
        QByteArray data = m_currentReply->readAll();

        filePath = m_saveDir + "/" + COUNTRY_FILE_NAME;
        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            success = true;
            qDebug() << "Downloaded country file to" << filePath;
        } else {
            emit errorOccurred("Failed to write file: " + filePath);
        }
    } else {
        emit errorOccurred("Download failed: " + m_currentReply->errorString());
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    emit downloadFinished(success, filePath);
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

int CountryFileDownloader::parseVersionFromHtml(const QString& html) {
    // Look for patterns like "CTY-3540" in the HTML
    static QRegularExpression re(R"(CTY-(\d{4}))");
    QRegularExpressionMatch match = re.match(html);

    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }

    // Fallback: use hardcoded version from Constants.h
    return CURRENT_CTY_VERSION;
}

} // namespace TR4QT
