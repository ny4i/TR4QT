/**
 * @file LogExportService.cpp
 * @brief Implementation of LogExportService
 *
 * Extracted from MainWindow::onEmailLogsToSupport() - 237 lines.
 */

#include "LogExportService.h"

#include <QMessageBox>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QAbstractButton>

#include "../logging/Logger.h"
#include "../logging/LogMacros.h"
#include "../utils/AppSettings.h"
#include "../utils/DialogHelper.h"
#include "../core/Constants.h"
#include "../radio/RadioController.h"

// Hamlib for radio model name lookup
#include <hamlib/rig.h>

namespace TR4QT {

LogExportResult LogExportService::exportLogsForSupport(QWidget* parentWidget, bool radioConnected) {
    LogExportResult result;

    // Get logs from last "PROGRAM STARTUP" banner forward
    QString logs = Logger::instance().getLastLogLines();

    // Build system info
    QString systemInfo = buildSystemInfo(radioConnected);

    // Build full log content
    QString logContent = systemInfo + "=== LOG (from last startup) ===\n\n" + logs;

    // Show preview dialog - user can cancel here
    if (!showPreviewDialog(parentWidget, logContent)) {
        result.cancelled = true;
        return result;
    }

    // Get Desktop path
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    // Create zip file
    QString zipFilePath = createZipFile(parentWidget, logContent, desktopPath);
    if (zipFilePath.isEmpty()) {
        result.errorMessage = "Failed to create zip file";
        return result;
    }

    result.zipFilePath = zipFilePath;
    result.success = true;

    // Show success dialog with options
    QString zipFileName = QFileInfo(zipFilePath).fileName();
    showSuccessDialog(parentWidget, zipFileName, zipFilePath, desktopPath);

    return result;
}

QString LogExportService::buildSystemInfo(bool radioConnected) const {
    // Get configured radio model from settings (uses profile system or legacy)
    RadioConfig radioConfig = AppSettings::instance().getActiveRadioConfig();
    QString configuredRadio = "None";
    QString connectionType = "None";
    QString connectionDetails = "";

    if (radioConfig.hamlibModelId > 0) {
        // Get radio model name from Hamlib
        const struct rig_caps* caps = rig_get_caps(radioConfig.hamlibModelId);
        if (caps) {
            configuredRadio = QString("%1 %2").arg(caps->mfg_name).arg(caps->model_name);
        } else {
            configuredRadio = QString("Unknown (ID: %1)").arg(radioConfig.hamlibModelId);
        }

        // Determine connection type without revealing IP addresses
        if (radioConfig.port.contains(':')) {
            connectionType = "Network (TCP)";
        } else if (!radioConfig.port.isEmpty()) {
            connectionType = "Serial";
            connectionDetails = QString("Port: %1, Baud: %2, %3%4%5")
                .arg(radioConfig.port)
                .arg(radioConfig.baudRate)
                .arg(radioConfig.dataBits)
                .arg(radioConfig.parity == 0 ? "N" : radioConfig.parity == 1 ? "O" : "E")
                .arg(radioConfig.stopBits);

            // Add CI-V address if configured (Icom radios)
            if (radioConfig.civAddress > 0) {
                connectionDetails += QString(", CI-V: 0x%1")
                    .arg(radioConfig.civAddress, 2, 16, QChar('0')).toUpper();
            }
        }
    }

    return QString(
        "TR4QT Version: %1\n"
        "Platform: %2 %3\n"
        "Qt Version: %4\n"
        "Radio Model (Configured): %5\n"
        "Connection Type: %6\n"
        "%7"
        "Poll Interval: %8 ms\n"
        "Radio Connected: %9\n"
        "\n"
    ).arg(APP_VERSION)
     .arg(QSysInfo::productType())
     .arg(QSysInfo::productVersion())
     .arg(QT_VERSION_STR)
     .arg(configuredRadio)
     .arg(connectionType)
     .arg(connectionDetails.isEmpty() ? "" : connectionDetails + "\n")
     .arg(radioConfig.pollInterval)
     .arg(radioConnected ? "Yes" : "No");
}

bool LogExportService::showPreviewDialog(QWidget* parent, const QString& logContent) const {
    QMessageBox preview;
    preview.setWindowTitle("Email Logs to Support - Preview");
    preview.setIcon(QMessageBox::Question);
    preview.setText(
        QString("This will create a zip file with your support logs (%1 characters).\n\n"
                "Click 'Show Details' below to review what will be included.\n\n"
                "The zip file will be saved to your Desktop for you to attach to an email.")
        .arg(logContent.length()));
    preview.setDetailedText(logContent);
    preview.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    preview.setDefaultButton(QMessageBox::Ok);

    // Auto-expand "Show Details"
    foreach (QAbstractButton *button, preview.buttons()) {
        if (preview.buttonRole(button) == QMessageBox::ActionRole) {
            button->click();
            break;
        }
    }

    return preview.exec() == QMessageBox::Ok;
}

QString LogExportService::createZipFile(QWidget* parent, const QString& logContent,
                                        const QString& desktopPath) const {
    // Generate filename with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HHmmss");
    QString logFileName = QString("tr4qt-logs-%1.txt").arg(timestamp);
    QString zipFileName = QString("tr4qt-logs-%1.zip").arg(timestamp);
    QString logFilePath = QFileInfo(desktopPath, logFileName).filePath();
    QString zipFilePath = QFileInfo(desktopPath, zipFileName).filePath();

    // Write log content to file
    QFile logFile(logFilePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        DialogHelper::critical(parent, "Error",
            QString("Failed to create temporary log file: %1\n\nError: %2")
            .arg(logFilePath)
            .arg(logFile.errorString()));
        return QString();
    }

    QTextStream out(&logFile);
    out << logContent;
    logFile.close();

    // Zip the log file
    QProcess zipProcess;
    zipProcess.setWorkingDirectory(desktopPath);

#ifdef Q_OS_WIN
    // Windows: Use PowerShell Compress-Archive
    zipProcess.start("powershell", QStringList()
        << "-Command"
        << QString("Compress-Archive -Path '%1' -DestinationPath '%2' -Force")
           .arg(logFileName).arg(zipFileName));
#else
    // macOS/Linux: Use zip command
    zipProcess.start("zip", QStringList() << "-j" << zipFileName << logFileName);
#endif

    if (!zipProcess.waitForFinished(5000)) {
        DialogHelper::critical(parent, "Error",
            "Failed to create zip file.\n\n"
            "Please manually attach the log file to your email:\n" + logFilePath);
        return QString();
    }

    if (zipProcess.exitCode() != 0) {
        DialogHelper::critical(parent, "Error",
            QString("Zip command failed with exit code %1.\n\n"
                    "Please manually attach the log file to your email:\n%2")
            .arg(zipProcess.exitCode())
            .arg(logFilePath));
        return QString();
    }

    // Delete the uncompressed log file (keep only the zip)
    QFile::remove(logFilePath);

    LOG_INFO("LogExportService", QString("Created support zip file: %1").arg(zipFilePath));
    return zipFilePath;
}

void LogExportService::showSuccessDialog(QWidget* parent, const QString& zipFileName,
                                         const QString& zipFilePath, const QString& desktopPath) const {
    QMessageBox instructions;
    instructions.setWindowTitle("Support Logs Ready");
    instructions.setIcon(QMessageBox::Information);
    instructions.setText(
        QString("Support logs saved to your Desktop:\n\n"
                "%1\n\n"
                "What would you like to do?")
        .arg(zipFileName));

    QPushButton* bothButton = instructions.addButton(
#ifdef Q_OS_MAC
        "Show in Finder && Open Email",
#else
        "Show in Explorer && Open Email",
#endif
        QMessageBox::AcceptRole);
    QPushButton* revealButton = instructions.addButton(
#ifdef Q_OS_MAC
        "Show in Finder Only",
#else
        "Show in Explorer Only",
#endif
        QMessageBox::ActionRole);
    QPushButton* emailButton = instructions.addButton("Open Email Only", QMessageBox::ActionRole);
    instructions.addButton("Close", QMessageBox::RejectRole);
    instructions.setDefaultButton(bothButton);

    instructions.exec();
    QAbstractButton* clicked = instructions.clickedButton();

    bool shouldReveal = (clicked == revealButton || clicked == bothButton);
    bool shouldEmail = (clicked == emailButton || clicked == bothButton);

    if (shouldReveal) {
        revealInFileManager(zipFilePath, desktopPath);
    }

    if (shouldEmail) {
        bool emailOpened = openEmailClient(parent, zipFileName);
        if (emailOpened && !shouldReveal) {
            DialogHelper::information(parent, "Don't Forget!",
                QString("Remember to attach the zip file from your Desktop:\n\n%1")
                .arg(zipFileName));
        }
    }
}

void LogExportService::revealInFileManager(const QString& filePath, const QString& dirPath) const {
#ifdef Q_OS_MAC
    // macOS: Use 'open -R' to reveal file in Finder
    QProcess::startDetached("open", QStringList() << "-R" << filePath);
#elif defined(Q_OS_WIN)
    // Windows: Use 'explorer /select,' to highlight file in Explorer
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(filePath));
#else
    // Linux: Open file manager at directory
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
#endif
    LOG_INFO("LogExportService", QString("Revealed support zip file: %1").arg(filePath));
}

bool LogExportService::openEmailClient(QWidget* parent, const QString& zipFileName) const {
    QString subject = QString("TR4QT Support Request - v%1 (%2)")
        .arg(APP_VERSION)
        .arg(QSysInfo::productType());

    QString body = QString(
        "Please describe your issue:\n\n\n\n"
        "---\n"
        "Logs attached: %1\n"
        "TR4QT Version: %2\n"
        "Platform: %3 %4")
        .arg(zipFileName)
        .arg(APP_VERSION)
        .arg(QSysInfo::productType())
        .arg(QSysInfo::productVersion());

    QString mailto = QString("mailto:support@ny4i.com?subject=%1&body=%2")
        .arg(QUrl::toPercentEncoding(subject))
        .arg(QUrl::toPercentEncoding(body));

    if (!QDesktopServices::openUrl(QUrl(mailto))) {
        DialogHelper::critical(parent, "Error",
            QString("Failed to open email client.\n\n"
                    "Please manually email the zip file to: support@ny4i.com\n\n"
                    "The file is on your Desktop:\n%1").arg(zipFileName));
        return false;
    }

    return true;
}

} // namespace TR4QT
