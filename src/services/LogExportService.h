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

/**
 * @file LogExportService.h
 * @brief Service for exporting and emailing support logs
 *
 * Extracted from MainWindow::onEmailLogsToSupport() - 237 lines.
 * Handles log collection, system info gathering, zip creation,
 * and user interaction for support requests.
 */

#ifndef LOGEXPORTSERVICE_H
#define LOGEXPORTSERVICE_H

#include <QString>
#include <QWidget>

namespace TR4QT {

/**
 * @brief Result of log export operation
 */
struct LogExportResult {
    bool success = false;
    bool cancelled = false;
    QString zipFilePath;
    QString errorMessage;
};

/**
 * @brief Service for creating and exporting support logs
 *
 * Handles:
 * - Log collection from Logger singleton
 * - System information gathering
 * - Radio configuration summary (without sensitive data)
 * - Zip file creation
 * - User preview and confirmation
 * - File reveal and email client launch
 */
class LogExportService {
public:
    LogExportService() = default;

    /**
     * @brief Export logs for support with user interaction
     *
     * Shows preview dialog, creates zip on Desktop, offers to
     * reveal in Finder/Explorer and open email client.
     *
     * @param parentWidget Parent for dialogs
     * @param radioConnected Whether radio is currently connected
     * @return LogExportResult with status and file path
     */
    LogExportResult exportLogsForSupport(QWidget* parentWidget, bool radioConnected);

private:
    /**
     * @brief Collect system and radio configuration info
     */
    QString buildSystemInfo(bool radioConnected) const;

    /**
     * @brief Show preview dialog and get user confirmation
     * @return true if user confirms, false if cancelled
     */
    bool showPreviewDialog(QWidget* parent, const QString& logContent) const;

    /**
     * @brief Create zip file from log content
     * @return Path to created zip, or empty string on failure
     */
    QString createZipFile(QWidget* parent, const QString& logContent,
                         const QString& desktopPath) const;

    /**
     * @brief Show success dialog with options (reveal, email, both)
     */
    void showSuccessDialog(QWidget* parent, const QString& zipFileName,
                          const QString& zipFilePath, const QString& desktopPath) const;

    /**
     * @brief Reveal file in Finder/Explorer
     */
    void revealInFileManager(const QString& filePath, const QString& dirPath) const;

    /**
     * @brief Open email client with pre-filled support request
     */
    bool openEmailClient(QWidget* parent, const QString& zipFileName) const;
};

} // namespace TR4QT

#endif // LOGEXPORTSERVICE_H
