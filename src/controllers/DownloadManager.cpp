#include "DownloadManager.h"
#include "../utils/CountryFile.h"
#include "../utils/CountryFileDownloader.h"
#include "../utils/LOTWUserDownloader.h"
#include "../utils/SCPDownloader.h"
#include "../utils/AppSettings.h"
#include "../utils/PathManager.h"
#include "../utils/DialogHelper.h"
#include "../logging/LogMacros.h"
#include <QProgressDialog>
#include <QEventLoop>
#include <QDateTime>
#include <QObject>

namespace TR4QT {

DownloadManager::DownloadManager(const Config& config, QWidget* parent)
    : QObject(parent)
    , m_config(config)
    , m_parent(parent)
{
}

DownloadManager::~DownloadManager()
{
}

CTYDownloadResult DownloadManager::downloadCTY(bool headless)
{
    LOG_DEBUG("DownloadManager", QString("Download CTY.dat - Starting download (headless=%1)").arg(headless));

    CTYDownloadResult result;

    // Validate configuration
    if (!m_config.countryFile) {
        result.success = false;
        result.errorMessage = "CountryFile not configured";
        LOG_ERROR("DownloadManager", "CTY download failed: CountryFile not configured");
        return result;
    }

    // Get the save directory (platform-native app data directory)
    QString saveDir = PathManager::getAppDataDir();

    // Create progress dialog (only if not headless)
    QProgressDialog* progressDialog = nullptr;
    if (!headless && m_parent) {
        const int PROGRESS_MAX = 100;
        progressDialog = new QProgressDialog("Downloading country file...", "Cancel", 0, PROGRESS_MAX, m_parent);
        progressDialog->setWindowTitle("Download CTY.dat");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);  // Don't auto-close when reaching 100%
        progressDialog->setAutoReset(false);  // Don't auto-reset when reaching 100%
        progressDialog->setValue(0);
    }

    // Create downloader
    CountryFileDownloader* downloader = new CountryFileDownloader(m_parent);

    // Event loop to wait for download completion
    QEventLoop eventLoop;

    // Connect progress signal (only if not headless)
    if (!headless && progressDialog) {
        const int KB_DIVISOR = 1024;
        QObject::connect(downloader, &CountryFileDownloader::downloadProgress,
                [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
                    if (progressDialog && bytesTotal > 0) {
                        const int PERCENTAGE_SCALE = 100;
                        int percentage = (bytesReceived * PERCENTAGE_SCALE) / bytesTotal;
                        progressDialog->setValue(percentage);
                        progressDialog->setLabelText(QString("Downloading country file... %1 KB / %2 KB")
                                                    .arg(bytesReceived / KB_DIVISOR)
                                                    .arg(bytesTotal / KB_DIVISOR));
                    }
                });
    }

    // Connect finished signal
    QObject::connect(downloader, &CountryFileDownloader::downloadFinished,
            [this, &result, &eventLoop, progressDialog, downloader, headless](
                bool success, const QString& filePath, const QString& version, int numericalVersion) {
                if (success) {
                    LOG_DEBUG("DownloadManager", QString("Download successful: %1 Version: %2 (CTY-%3)")
                        .arg(filePath).arg(version).arg(numericalVersion));

                    // Auto-reload the country file
                    if (m_config.countryFile->loadFromFile(filePath)) {
                        // Set the version from the download
                        m_config.countryFile->setVersion(version);
                        LOG_DEBUG("DownloadManager", QString("Country file reloaded successfully. Version: %1")
                            .arg(m_config.countryFile->getVersion()));

                        // Save the numerical version to AppSettings to prevent re-notification
                        if (numericalVersion > 0) {
                            AppSettings::instance().setCountryFileVersion(numericalVersion);
                            LOG_DEBUG("DownloadManager", QString("Saved CTY version to settings: %1").arg(numericalVersion));
                        }

                        const int STATUS_TIMEOUT_MS = 5000;
                        result.success = true;
                        result.version = version;
                        result.numericalVersion = numericalVersion;
                        result.statusMessage = QString("CTY.DAT %1 loaded successfully").arg(version);

                        if (progressDialog) {
                            // Update progress dialog to show completion (user clicks OK to dismiss)
                            const int PROGRESS_COMPLETE = 100;
                            progressDialog->setLabelText(QString("Country file downloaded and loaded successfully!\n\nVersion: %1").arg(version));
                            progressDialog->setCancelButtonText("OK");
                            progressDialog->setValue(PROGRESS_COMPLETE);

                            // Wait for user to click OK, then close
                            QObject::connect(progressDialog, &QProgressDialog::canceled, progressDialog, &QProgressDialog::deleteLater);
                        }
                    } else {
                        LOG_WARN("DownloadManager", "Failed to reload country file after download");
                        result.success = false;
                        result.errorMessage = "Failed to reload country file after download";
                        result.statusMessage = "Failed to reload CTY.DAT";

                        if (progressDialog) {
                            progressDialog->close();
                            progressDialog->deleteLater();
                        }

                        // Show error dialog only if reload fails
                        if (!headless && m_parent) {
                            DialogHelper::warning(m_parent, "Reload Failed",
                                "Failed to reload the country file after download.\n\n"
                                "Please restart the application.");
                        }
                    }
                } else {
                    result.success = false;
                    result.errorMessage = "Failed to download country file";

                    if (progressDialog) {
                        progressDialog->close();
                        progressDialog->deleteLater();
                    }
                    if (!headless && m_parent) {
                        DialogHelper::critical(m_parent, "Download Failed",
                            "Failed to download country file.\n\n"
                            "Please check your internet connection and try again.");
                    } else {
                        LOG_WARN("DownloadManager", "Failed to download country file (headless)");
                    }
                }

                downloader->deleteLater();
                eventLoop.quit();
            });

    // Connect error signal (only if not headless)
    if (!headless && progressDialog) {
        QObject::connect(downloader, &CountryFileDownloader::errorOccurred,
                [progressDialog](const QString& error) {
                    LOG_DEBUG("DownloadManager", QString("Download error: %1").arg(error));
                    if (progressDialog) {
                        progressDialog->setLabelText("Error: " + error);
                    }
                });

        // Connect cancel button
        QObject::connect(progressDialog, &QProgressDialog::canceled,
                downloader, &CountryFileDownloader::cancel);
    }

    // Start download
    downloader->downloadLatest(saveDir);

    // Wait for download to complete
    eventLoop.exec();

    // Emit signal so MainWindow can clear "update available" status
    emit ctyDownloadCompleted(result.success);

    return result;
}

LOTWDownloadResult DownloadManager::downloadLOTW(bool headless)
{
    LOG_DEBUG("DownloadManager", QString("Download LOTW Users - Starting download (headless=%1)").arg(headless));

    LOTWDownloadResult result;

    // Create progress dialog (only if not headless)
    QProgressDialog* progressDialog = nullptr;
    if (!headless && m_parent) {
        const int PROGRESS_MAX = 100;
        progressDialog = new QProgressDialog("Downloading LOTW user list...", "Cancel", 0, PROGRESS_MAX, m_parent);
        progressDialog->setWindowTitle("Download LOTW Users");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);  // Don't auto-close when reaching 100%
        progressDialog->setAutoReset(false);  // Don't auto-reset when reaching 100%
        progressDialog->setValue(0);
    }

    // Create downloader
    LOTWUserDownloader* downloader = new LOTWUserDownloader(m_parent);

    // Event loop to wait for download completion
    QEventLoop eventLoop;

    // Connect progress signal (only if not headless)
    if (!headless && progressDialog) {
        const int KB_DIVISOR = 1024;
        QObject::connect(downloader, &LOTWUserDownloader::downloadProgress,
                [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
                    if (progressDialog && bytesTotal > 0) {
                        const int PERCENTAGE_SCALE = 100;
                        int percentage = (bytesReceived * PERCENTAGE_SCALE) / bytesTotal;
                        progressDialog->setValue(percentage);
                        progressDialog->setLabelText(QString("Downloading LOTW user list... %1 KB / %2 KB")
                                                    .arg(bytesReceived / KB_DIVISOR)
                                                    .arg(bytesTotal / KB_DIVISOR));
                    }
                });
    }

    // Connect finished signal
    QObject::connect(downloader, &LOTWUserDownloader::downloadFinished,
            [this, &result, &eventLoop, progressDialog, downloader, headless](
                bool success, int userCount, const QString& error) {
                if (success) {
                    LOG_DEBUG("DownloadManager", QString("LOTW download successful: %1 users imported").arg(userCount));

                    // Update last update timestamp
                    AppSettings& settings = AppSettings::instance();
                    settings.setLotwLastUpdateTime(QDateTime::currentDateTime());

                    result.success = true;
                    result.userCount = userCount;
                    result.statusMessage = QString("LOTW user list downloaded: %1 users imported").arg(userCount);

                    if (progressDialog) {
                        // Update progress dialog to show completion (user clicks OK to dismiss)
                        const int PROGRESS_COMPLETE = 100;
                        progressDialog->setLabelText(QString("LOTW user list downloaded successfully!\n\n%1 users imported").arg(userCount));
                        progressDialog->setCancelButtonText("OK");
                        progressDialog->setValue(PROGRESS_COMPLETE);

                        // Wait for user to click OK, then close
                        QObject::connect(progressDialog, &QProgressDialog::canceled, progressDialog, &QProgressDialog::deleteLater);
                    }
                } else {
                    result.success = false;
                    result.errorMessage = error;

                    if (progressDialog) {
                        progressDialog->close();
                        progressDialog->deleteLater();
                    }
                    if (!headless && m_parent) {
                        DialogHelper::critical(m_parent, "Download Failed",
                            QString("Failed to download LOTW user list.\n\n%1\n\n"
                                   "Please check your internet connection and try again.").arg(error));
                    } else {
                        LOG_WARN("DownloadManager", QString("Failed to download LOTW user list (headless): %1").arg(error));
                    }
                }

                downloader->deleteLater();
                eventLoop.quit();
            });

    // Connect error signal (only if not headless)
    if (!headless && progressDialog) {
        QObject::connect(downloader, &LOTWUserDownloader::errorOccurred,
                [progressDialog](const QString& error) {
                    LOG_DEBUG("DownloadManager", QString("LOTW download error: %1").arg(error));
                    if (progressDialog) {
                        progressDialog->setLabelText("Error: " + error);
                    }
                });

        // Connect cancel button
        QObject::connect(progressDialog, &QProgressDialog::canceled,
                downloader, &LOTWUserDownloader::cancel);
    }

    // Start download
    downloader->downloadLatest();

    // Wait for download to complete
    eventLoop.exec();

    return result;
}

SCPDownloadResult DownloadManager::downloadSCP(bool headless)
{
    LOG_DEBUG("DownloadManager", QString("Download MASTER.SCP - Starting download (headless=%1)").arg(headless));

    SCPDownloadResult result;

    // Create progress dialog (only if not headless)
    QProgressDialog* progressDialog = nullptr;
    if (!headless && m_parent) {
        const int PROGRESS_MAX = 100;
        progressDialog = new QProgressDialog("Downloading MASTER.SCP...", "Cancel", 0, PROGRESS_MAX, m_parent);
        progressDialog->setWindowTitle("Download MASTER.SCP");
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);  // Don't auto-close when reaching 100%
        progressDialog->setAutoReset(false);  // Don't auto-reset when reaching 100%
        progressDialog->setValue(0);
    }

    // Create downloader
    SCPDownloader* downloader = new SCPDownloader(m_parent);

    // Event loop to wait for download completion
    QEventLoop eventLoop;

    // Connect progress signal (only if not headless)
    if (!headless && progressDialog) {
        const int KB_DIVISOR = 1024;
        QObject::connect(downloader, &SCPDownloader::downloadProgress,
                [progressDialog](qint64 bytesReceived, qint64 bytesTotal) {
                    if (progressDialog && bytesTotal > 0) {
                        const int PERCENTAGE_SCALE = 100;
                        int percentage = (bytesReceived * PERCENTAGE_SCALE) / bytesTotal;
                        progressDialog->setValue(percentage);
                        progressDialog->setLabelText(QString("Downloading MASTER.SCP... %1 KB / %2 KB")
                                                    .arg(bytesReceived / KB_DIVISOR)
                                                    .arg(bytesTotal / KB_DIVISOR));
                    }
                });
    }

    // Connect finished signal
    QObject::connect(downloader, &SCPDownloader::downloadFinished,
            [this, &result, &eventLoop, progressDialog, downloader, headless](
                bool success, int callsignCount, const QString& error) {
                if (success) {
                    LOG_DEBUG("DownloadManager", QString("SCP download successful: %1 callsigns imported").arg(callsignCount));

                    // Update last update timestamp
                    AppSettings& settings = AppSettings::instance();
                    settings.setSCPLastUpdate(QDateTime::currentDateTime());

                    result.success = true;
                    result.callsignCount = callsignCount;
                    result.statusMessage = QString("MASTER.SCP downloaded: %1 callsigns imported").arg(callsignCount);

                    if (progressDialog) {
                        // Update progress dialog to show completion (user clicks OK to dismiss)
                        const int PROGRESS_COMPLETE = 100;
                        progressDialog->setLabelText(QString("MASTER.SCP downloaded successfully!\n\n%1 callsigns imported").arg(callsignCount));
                        progressDialog->setCancelButtonText("OK");
                        progressDialog->setValue(PROGRESS_COMPLETE);

                        // Wait for user to click OK, then close
                        QObject::connect(progressDialog, &QProgressDialog::canceled, progressDialog, &QProgressDialog::deleteLater);
                    }
                } else {
                    result.success = false;
                    result.errorMessage = error;

                    if (progressDialog) {
                        progressDialog->close();
                        progressDialog->deleteLater();
                    }
                    if (!headless && m_parent) {
                        DialogHelper::critical(m_parent, "Download Failed",
                            QString("Failed to download MASTER.SCP.\n\n%1\n\n"
                                   "Please check your internet connection and try again.").arg(error));
                    } else {
                        LOG_WARN("DownloadManager", QString("Failed to download MASTER.SCP (headless): %1").arg(error));
                    }
                }

                downloader->deleteLater();
                eventLoop.quit();
            });

    // Connect error signal (only if not headless)
    if (!headless && progressDialog) {
        QObject::connect(downloader, &SCPDownloader::errorOccurred,
                [progressDialog](const QString& error) {
                    LOG_DEBUG("DownloadManager", QString("SCP download error: %1").arg(error));
                    if (progressDialog) {
                        progressDialog->setLabelText("Error: " + error);
                    }
                });

        // Connect cancel button
        QObject::connect(progressDialog, &QProgressDialog::canceled,
                downloader, &SCPDownloader::cancel);
    }

    // Start download
    downloader->downloadLatest();

    // Wait for download to complete
    eventLoop.exec();

    return result;
}

void DownloadManager::updateConfig(const Config& config)
{
    m_config = config;
}

} // namespace TR4QT
