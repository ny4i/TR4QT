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

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QString>
#include <QWidget>

namespace TR4QT {

// Forward declarations
class CountryFile;

/**
 * Result structure for CTY.DAT download operations
 */
struct CTYDownloadResult {
    bool success = false;
    QString errorMessage;
    QString version;           // Version string (e.g., "VER20251218")
    int numericalVersion = 0;  // Numerical version (e.g., 3541)
    QString statusMessage;
};

/**
 * Result structure for LOTW user list download operations
 */
struct LOTWDownloadResult {
    bool success = false;
    QString errorMessage;
    int userCount = 0;
    QString statusMessage;
};

/**
 * Result structure for MASTER.SCP download operations
 */
struct SCPDownloadResult {
    bool success = false;
    QString errorMessage;
    int callsignCount = 0;
    QString statusMessage;
};

/**
 * DownloadManager
 *
 * Handles downloading and updating data files:
 * - CTY.DAT (country file)
 * - LOTW user list
 * - MASTER.SCP (super check partial)
 *
 * Responsibilities:
 * - Download files from their respective URLs
 * - Show progress dialogs during downloads (unless headless mode)
 * - Parse and import downloaded data
 * - Update AppSettings with timestamps/versions
 * - Reload CountryFile after CTY.DAT download
 * - Emit signals when downloads complete (for UI updates)
 *
 * Thread Safety:
 * - All methods must be called from the Qt main/UI thread
 * - Uses Qt's signal/slot mechanism for async downloads
 * - Progress dialogs are modal (blocks UI thread)
 */
class DownloadManager : public QObject {
    Q_OBJECT

public:
    /**
     * Configuration for DownloadManager
     */
    struct Config {
        CountryFile* countryFile = nullptr;  // Required for CTY downloads (will reload after download)
    };

    /**
     * Construct a DownloadManager
     * @param config Configuration with dependencies
     * @param parent Parent widget for dialogs (nullptr for headless)
     */
    explicit DownloadManager(const Config& config, QWidget* parent = nullptr);

    /**
     * Destructor
     */
    ~DownloadManager();

    /**
     * Download and update CTY.DAT (country file)
     *
     * Downloads from https://www.country-files.com/,
     * saves to platform-native app data directory,
     * reloads CountryFile, updates AppSettings.
     *
     * @param headless If true, no progress dialog or error dialogs shown
     * @return CTYDownloadResult with success/error, version, and status
     */
    CTYDownloadResult downloadCTY(bool headless = false);

    /**
     * Download and update LOTW user list
     *
     * Downloads from https://lotw.arrl.org/lotw-user-activity.csv,
     * parses CSV, imports to GlobalDatabase,
     * updates AppSettings with timestamp.
     *
     * @param headless If true, no progress dialog or error dialogs shown
     * @return LOTWDownloadResult with success/error, user count, and status
     */
    LOTWDownloadResult downloadLOTW(bool headless = false);

    /**
     * Download and update MASTER.SCP
     *
     * Downloads from http://www.supercheckpartial.com/MASTER.SCP,
     * parses plain text, imports to GlobalDatabase,
     * updates AppSettings with timestamp.
     *
     * @param headless If true, no progress dialog or error dialogs shown
     * @return SCPDownloadResult with success/error, callsign count, and status
     */
    SCPDownloadResult downloadSCP(bool headless = false);

    /**
     * Update configuration (for when CountryFile changes)
     * @param config New configuration
     */
    void updateConfig(const Config& config);

signals:
    /**
     * Emitted when CTY.DAT download completes (success or failure)
     * MainWindow can connect to this to clear "update available" status
     * @param success true if download and reload succeeded
     */
    void ctyDownloadCompleted(bool success);

private:
    Config m_config;
    QWidget* m_parent;
};

} // namespace TR4QT

#endif // DOWNLOADMANAGER_H
