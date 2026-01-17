#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QFont>

namespace TR4QT {

// Forward declarations
struct UdpDestination;

/**
 * Font configuration for all UI elements
 */
struct FontConfig {
    QFont entryFont;        // Callsign/exchange entry fields
    QFont tableFont;        // QSO table
    QFont miscFont;         // Stats panel labels (This Hr, Rate, Op, etc.)
    int gridFontSize;       // Band summary grid
    int scpFontSize;        // SCP matches label
};

/**
 * Window geometry configuration
 */
struct WindowGeometry {
    QByteArray mainWindowGeometry;
    QByteArray mainWindowState;
    bool dxClusterVisible = false;
    QByteArray dxClusterGeometry;
    bool bandMapVisible = false;
    QByteArray bandMapGeometry;
    bool radioControlVisible = false;
    QByteArray radioControlGeometry;
    bool multipliersVisible = false;
    QByteArray multipliersGeometry;
    bool statisticsVisible = false;
    QByteArray statisticsGeometry;
    bool sectionsMapVisible = false;
    bool statesMapVisible = false;
    bool graylineMapVisible = false;
    QByteArray graylineMapGeometry;
    QString currentOperator;
};

/**
 * UDP Broadcast configuration
 */
struct UdpBroadcastConfig {
    bool enabled;
    bool radioInfoEnabled;
    bool contactInfoEnabled;
    int throttleInterval;
    QList<UdpDestination> destinations;
};

/**
 * Backup configuration
 */
struct BackupConfig {
    bool autoBackupEnabled;
    int autoBackupInterval;     // QSO count between backups
    QString backupDirectory;
    int maxBackups;
};

/**
 * SettingsManager - Extract settings management from MainWindow
 *
 * Provides a clean interface for loading/saving application settings
 * without tight coupling to MainWindow implementation details.
 *
 * Uses AppSettings singleton for persistence, but returns structured
 * configuration objects that MainWindow can apply to widgets.
 *
 * Philosophy:
 * - Manager loads/saves settings
 * - MainWindow applies settings to widgets
 * - No direct widget manipulation from manager
 *
 * Usage:
 *   SettingsManager manager;
 *   FontConfig fonts = manager.loadFontConfig();
 *   m_callsignEntry->setFont(fonts.entryFont);
 */
class SettingsManager {
public:
    SettingsManager() = default;
    ~SettingsManager() = default;

    // Font configuration
    FontConfig loadFontConfig() const;

    // Window geometry and visibility
    WindowGeometry loadWindowGeometry() const;
    void saveWindowGeometry(const WindowGeometry& geometry);

    // UDP Broadcast configuration
    UdpBroadcastConfig loadUdpBroadcastConfig() const;

    // Backup configuration
    BackupConfig loadBackupConfig() const;

    // QSO table column widths (per-contest)
    QList<int> loadQSOTableColumnWidths(const QString& contestId) const;
    void saveQSOTableColumnWidths(const QString& contestId, const QList<int>& widths);

private:
    // Prevent copying
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
};

} // namespace TR4QT

#endif // SETTINGSMANAGER_H
